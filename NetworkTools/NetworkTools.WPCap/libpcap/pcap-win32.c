/*
 * Copyright (c) 1999 - 2005 NetGroup, Politecnico di Torino (Italy)
 * Copyright (c) 2005 - 2008 CACE Technologies, Davis (California)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Politecnico di Torino, CACE Technologies 
 * nor the names of its contributors may be used to endorse or promote 
 * products derived from this software without specific prior written 
 * permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef lint
static const char rcsid[] _U_ =
    "@(#) $Header: /tcpdump/master/libpcap/pcap-win32.c,v 1.34.2.8 2008-05-21 22:11:26 gianluca Exp $ (LBL)";
#endif

#include <pcap-int.h>
#include <Packet32.h>
#ifdef __MINGW32__
#include <ddk/ndis.h>
#else /*__MINGW32__*/
#include <ntddndis.h>
#endif /*__MINGW32__*/
#ifdef HAVE_DAG_API
#include <dagnew.h>
#include <dagapi.h>
#endif /* HAVE_DAG_API */
#ifdef __MINGW32__
int* _errno();
#define errno (*_errno())
#endif /* __MINGW32__ */

#ifdef HAVE_REMOTE
#include <pcap-remote.h>
#endif	/* HAVE_REMOTE */

#include "pcap-tc.h"

static int pcap_setfilter_win32_npf(pcap_t *, struct bpf_program *);
static int pcap_setfilter_win32_dag(pcap_t *, struct bpf_program *);
static int pcap_getnonblock_win32(pcap_t *, char *);
static int pcap_setnonblock_win32(pcap_t *, int, char *);

/*dimension of the buffer in the pcap_t structure*/
#define	WIN32_DEFAULT_USER_BUFFER_SIZE 256000

/*dimension of the buffer in the kernel driver NPF */
#define	WIN32_DEFAULT_KERNEL_BUFFER_SIZE 1000000

/* Equivalent to ntohs(), but a lot faster under Windows */
#define SWAPS(_X) ((_X & 0xff) << 8) | (_X >> 8)

/*
 * Header that the NetworkTools driver associates to the packets.
 * Once was in bpf.h
 */
struct bpf_hdr {
	struct timeval	bh_tstamp;	/* time stamp */
	bpf_u_int32	bh_caplen;	/* length of captured portion */
	bpf_u_int32	bh_datalen;	/* original length of packet */
	u_short		bh_hdrlen;	/* length of bpf header (this struct
					   plus alignment padding) */
};

CRITICAL_SECTION g_PcapCompileCriticalSection;

BOOL WINAPI DllMain(
  HANDLE hinstDLL,
  DWORD dwReason,
  LPVOID lpvReserved
)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		InitializeCriticalSection(&g_PcapCompileCriticalSection);
	}

	return TRUE;
}

/* Start winsock */
int 
wsockinit()
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD( 1, 1); 
	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 )
	{
		return -1;
	}
	return 0;
}


static int
pcap_stats_win32(pcap_t *p, struct pcap_stat *ps)
{

	if(PacketGetStats(p->adapter, (struct bpf_stat*)ps) != TRUE){
		snprintf(p->errbuf, PCAP_ERRBUF_SIZE, "PacketGetStats error: %s", pcap_win32strerror());
		return -1;
	}

	return 0;
}

/* Set the dimension of the kernel-level capture buffer */
static int
pcap_setbuff_win32(pcap_t *p, int dim)
{
#ifdef HAVE_REMOTE
	if (p->rmt_clientside)
	{
		/* Currently, this is a bug: the capture buffer cannot be set with remote capture */
		return 0;
	}
 #endif        /* HAVE_REMOTE */

	if(PacketSetBuff(p->adapter,dim)==FALSE)
	{
		snprintf(p->errbuf, PCAP_ERRBUF_SIZE, "driver error: not enough memory to allocate the kernel buffer");
		return -1;
	}
	return 0;
}

/* Set the driver working mode */
static int
pcap_setmode_win32(pcap_t *p, int mode)
{
	if(PacketSetMode(p->adapter,mode)==FALSE)
	{
		snprintf(p->errbuf, PCAP_ERRBUF_SIZE, "driver error: working mode not recognized");
		return -1;
	}

	return 0;
}

/*set the minimum amount of data that will release a read call*/
static int
pcap_setmintocopy_win32(pcap_t *p, int size)
{
	if(PacketSetMinToCopy(p->adapter, size)==FALSE)
	{
		snprintf(p->errbuf, PCAP_ERRBUF_SIZE, "driver error: unable to set the requested mintocopy size");
		return -1;
	}
	return 0;
}

static int
pcap_read_win32_npf(pcap_t *p, int cnt, pcap_handler callback, u_char *user)
{
	int cc;
	int n = 0;
	register u_char *bp, *ep;

#ifdef HAVE_REMOTE
	static int samp_npkt;				// parameter needed for sampling, with '1 out of N' method has been requested
	static struct timeval samp_time;	// parameter needed for sampling, with '1 every N ms' method has been requested
#endif	/* HAVE_REMOTE */

	cc = p->cc;
	if (p->cc == 0) {
		/*
		 * Has "pcap_breakloop()" been called?
		 */
		if (p->break_loop) {
			/*
			 * Yes - clear the flag that indicates that it
			 * has, and return -2 to indicate that we were
			 * told to break out of the loop.
			 */
			p->break_loop = 0;
			return (-2);
		}

	    /* capture the packets */
		if(PacketReceivePacket(p->adapter,p->Packet,TRUE)==FALSE){
			snprintf(p->errbuf, PCAP_ERRBUF_SIZE, "read error: PacketReceivePacket failed");
			return (-1);
		}
			
		cc = p->Packet->ulBytesReceived;

		bp = p->Packet->Buffer;
	} 
	else
		bp = p->bp;

	/*
	 * Loop through each packet.
	 */
#define bhp ((struct bpf_hdr *)bp)
	ep = bp + cc;
	while (1) {
		register int caplen, hdrlen;

		/*
		 * Has "pcap_breakloop()" been called?
		 * If so, return immediately - if we haven't read any
		 * packets, clear the flag and return -2 to indicate
		 * that we were told to break out of the loop, otherwise
		 * leave the flag set, so that the *next* call will break
		 * out of the loop without having read any packets, and
		 * return the number of packets we've processed so far.
		 */
		if (p->break_loop) {
			if (n == 0) {
				p->break_loop = 0;
				return (-2);
			} else {
				p->bp = bp;
				p->cc = ep - bp;
				return (n);
			}
		}
		if (bp >= ep)
			break;

		caplen = bhp->bh_caplen;
		hdrlen = bhp->bh_hdrlen;

#ifdef HAVE_REMOTE
		if (p->rmt_samp.method == PCAP_SAMP_1_EVERY_N)
		{
			samp_npkt= (samp_npkt + 1) % p->rmt_samp.value;

			// Discard all packets that are not '1 out of N'
			if (samp_npkt != 0)
			{
				bp += BPF_WORDALIGN(caplen + hdrlen);
				continue;
			}
		}

		if (p->rmt_samp.method == PCAP_SAMP_FIRST_AFTER_N_MS)
		{
		struct pcap_pkthdr *pkt_header= (struct pcap_pkthdr*) bp;

			// Check if the timestamp of the arrived packet is smaller than our target time
			if ( (pkt_header->ts.tv_sec < samp_time.tv_sec) ||
					( (pkt_header->ts.tv_sec == samp_time.tv_sec) && (pkt_header->ts.tv_usec < samp_time.tv_usec) ) )
			{
				bp += BPF_WORDALIGN(caplen + hdrlen);
				continue;
			}

			// The arrived packet is suitable for being sent to the remote host
			// So, let's update the target time
			samp_time.tv_usec= pkt_header->ts.tv_usec + p->rmt_samp.value * 1000;
			if (samp_time.tv_usec > 1000000)
			{
				samp_time.tv_sec= pkt_header->ts.tv_sec + samp_time.tv_usec / 1000000;
				samp_time.tv_usec= samp_time.tv_usec % 1000000;
			}
		}
#endif	/* HAVE_REMOTE */

		/*
		 * XXX A bpf_hdr matches a pcap_pkthdr.
		 */
		(*callback)(user, (struct pcap_pkthdr*)bp, bp + hdrlen);
		bp += BPF_WORDALIGN(caplen + hdrlen);
		if (++n >= cnt && cnt > 0) {
			p->bp = bp;
			p->cc = ep - bp;
			return (n);
		}
	}
#undef bhp
	p->cc = 0;
	return (n);
}

#ifdef HAVE_DAG_API
static int
pcap_read_win32_dag(pcap_t *p, int cnt, pcap_handler callback, u_char *user)
{
	u_char *dp = NULL;
	int	packet_len = 0, caplen = 0;
	struct pcap_pkthdr	pcap_header;
	u_char *endofbuf;
	int n = 0;
	dag_record_t *header;
	unsigned erf_record_len;
	ULONGLONG ts;
	int cc;
	unsigned swt;
	unsigned dfp = p->adapter->DagFastProcess;

	cc = p->cc;
	if (cc == 0) /* Get new packets only if we have processed all the ones of the previous read */
	{
	    /* Get new packets from the network */
		if(PacketReceivePacket(p->adapter, p->Packet, TRUE)==FALSE){
			snprintf(p->errbuf, PCAP_ERRBUF_SIZE, "read error: PacketReceivePacket failed");
			return (-1);
		}

		cc = p->Packet->ulBytesReceived;
		if(cc == 0)
			/* The timeout has expired but we no packets arrived */
			return 0;
		header = (dag_record_t*)p->adapter->DagBuffer;
	} 
	else
		header = (dag_record_t*)p->bp;
	
	endofbuf = (char*)header + cc;
	
	/* 
	 * Cycle through the packets 
	 */
	do
	{
		erf_record_len = SWAPS(header->rlen);
		if((char*)header + erf_record_len > endofbuf)
			break;

		/* Increase the number of captured packets */
		p->md.stat.ps_recv++;
		
		/* Find the beginning of the packet */
		dp = ((u_char *)header) + dag_record_size;

		/* Determine actual packet len */
		switch(header->type) 
		{
		case TYPE_ATM:
			packet_len = ATM_SNAPLEN;
			caplen = ATM_SNAPLEN;
			dp += 4;

			break;

		case TYPE_ETH:
			swt = SWAPS(header->wlen);
			packet_len = swt - (p->md.dag_fcs_bits);
			caplen = erf_record_len - dag_record_size - 2;
			if (caplen > packet_len)
			{
				caplen = packet_len;
			}
			dp += 2;
			
			break;
		
		case TYPE_HDLC_POS:
			swt = SWAPS(header->wlen);
			packet_len = swt - (p->md.dag_fcs_bits);
			caplen = erf_record_len - dag_record_size;
			if (caplen > packet_len)
			{
				caplen = packet_len;
			}
			
			break;
		}
		
		if(caplen > p->snapshot)
			caplen = p->snapshot;

		/*
		 * Has "pcap_breakloop()" been called?
		 * If so, return immediately - if we haven't read any
		 * packets, clear the flag and return -2 to indicate
		 * that we were told to break out of the loop, otherwise
		 * leave the flag set, so that the *next* call will break
		 * out of the loop without having read any packets, and
		 * return the number of packets we've processed so far.
		 */
		if (p->break_loop) 
		{
			if (n == 0) 
			{
				p->break_loop = 0;
				return (-2);
			} 
			else 
			{
				p->bp = (char*)header;
				p->cc = endofbuf - (char*)header;
				return (n);
			}
		}

		if(!dfp)
		{
			/* convert between timestamp formats */
			ts = header->ts;
			pcap_header.ts.tv_sec = (int)(ts >> 32);
			ts = (ts & 0xffffffffi64) * 1000000;
			ts += 0x80000000; /* rounding */
			pcap_header.ts.tv_usec = (int)(ts >> 32);
			if (pcap_header.ts.tv_usec >= 1000000) {
				pcap_header.ts.tv_usec -= 1000000;
				pcap_header.ts.tv_sec++;
			}
		}
		
		/* No underlaying filtering system. We need to filter on our own */
		if (p->fcode.bf_insns) 
		{
			if (bpf_filter(p->fcode.bf_insns, dp, packet_len, caplen) == 0) 
			{
				/* Move to next packet */
				header = (dag_record_t*)((char*)header + erf_record_len);
				continue;
			}
		}
		
		/* Fill the header for the user suppplied callback function */
		pcap_header.caplen = caplen;
		pcap_header.len = packet_len;
		
		/* Call the callback function */
		(*callback)(user, &pcap_header, dp);
		
		/* Move to next packet */
		header = (dag_record_t*)((char*)header + erf_record_len);

		/* Stop if the number of packets requested by user has been reached*/
		if (++n >= cnt && cnt > 0) 
		{
			p->bp = (char*)header;
			p->cc = endofbuf - (char*)header;
			return (n);
		}
	}
	while((u_char*)header < endofbuf);
	
  return 1;
}
#endif /* HAVE_DAG_API */

/* Send a packet to the network */
static int 
pcap_inject_win32(pcap_t *p, const void *buf, size_t size){
	LPPACKET PacketToSend;

	PacketToSend=PacketAllocatePacket();
	
	if (PacketToSend == NULL)
	{
		snprintf(p->errbuf, PCAP_ERRBUF_SIZE, "send error: PacketAllocatePacket failed");
		return -1;
	}
	
	PacketInitPacket(PacketToSend,(PVOID)buf,size);
	if(PacketSendPacket(p->adapter,PacketToSend,TRUE) == FALSE){
		snprintf(p->errbuf, PCAP_ERRBUF_SIZE, "send error: PacketSendPacket failed");
		PacketFreePacket(PacketToSend);
		return -1;
	}

	PacketFreePacket(PacketToSend);

	/*
	 * We assume it all got sent if "PacketSendPacket()" succeeded.
	 * "pcap_inject()" is expected to return the number of bytes
	 * sent.
	 */
	return size;
}

static void
pcap_cleanup_win32(pcap_t *p)
{
	if (p->adapter != NULL) {
		PacketCloseAdapter(p->adapter);
		p->adapter = NULL;
	}
	if (p->Packet) {
		PacketFreePacket(p->Packet);
		p->Packet = NULL;
	}
	pcap_cleanup_live_common(p);
}

static int
pcap_activate_win32(pcap_t *p)
{
	NetType type;

#ifdef HAVE_REMOTE
	char host[PCAP_BUF_SIZE + 1];
	char port[PCAP_BUF_SIZE + 1];
	char name[PCAP_BUF_SIZE + 1];
	int srctype;
	int opensource_remote_result;

	/*
		Retrofit; we have to make older applications compatible with the remote capture
		So, we're calling the pcap_open_remote() from here, that is a very dirty thing.
		Obviously, we cannot exploit all the new features; for instance, we cannot
		send authentication, we cannot use a UDP data connection, and so on.
	*/
	if (pcap_parsesrcstr(p->opt.source, &srctype, host, port, name, p->errbuf) )
		return PCAP_ERROR;

	if (srctype == PCAP_SRC_IFREMOTE)
	{
		opensource_remote_result = pcap_opensource_remote(p, NULL);

		if (opensource_remote_result != 0) 
			return opensource_remote_result;

		p->rmt_flags= (p->opt.promisc) ? PCAP_OPENFLAG_PROMISCUOUS : 0;

		return 0;
	}

	if (srctype == PCAP_SRC_IFLOCAL)
	{
		/*
		 * If it starts with rpcap://, cut down the string
		 */
		if (strncmp(p->opt.source, PCAP_SRC_IF_STRING, strlen(PCAP_SRC_IF_STRING)) == 0)
		{
			size_t len = strlen(p->opt.source) - strlen(PCAP_SRC_IF_STRING) + 1;
			char *new_string;
			/*
			 * allocate a new string and free the old one
			 */
			if (len > 0)
			{
				new_string = (char*)malloc(len);
				if (new_string != NULL)
				{
					char *tmp;
					strcpy(new_string, p->opt.source + strlen(PCAP_SRC_IF_STRING));
					tmp = p->opt.source;
					p->opt.source = new_string;
					free(tmp);
				}
			}
		}
	}

#endif	/* HAVE_REMOTE */

	if (p->opt.rfmon) {
		/*
		 * No monitor mode on Windows.  It could be done on
		 * Vista with drivers that support the native 802.11
		 * mechanism and monitor mode.
		 */
		return (PCAP_ERROR_RFMON_NOTSUP);
	}

	/* Init WinSock */
	wsockinit();

	p->adapter = PacketOpenAdapter(p->opt.source);
	
	if (p->adapter == NULL)
	{
		/* Adapter detected but we are not able to open it. Return failure. */
		snprintf(p->errbuf, PCAP_ERRBUF_SIZE, "Error opening adapter: %s", pcap_win32strerror());
		return PCAP_ERROR;
	}
	
	/*get network type*/
	if(PacketGetNetType (p->adapter,&type) == FALSE)
	{
		snprintf(p->errbuf, PCAP_ERRBUF_SIZE, "Cannot determine the network type: %s", pcap_win32strerror());
		goto bad;
	}
	
	/*Set the linktype*/
	switch (type.LinkType) 
	{
	case NdisMediumWan:
		p->linktype = DLT_EN10MB;
		break;
		
	case NdisMedium802_3:
		p->linktype = DLT_EN10MB;
		/*
		 * This is (presumably) a real Ethernet capture; give it a
		 * link-layer-type list with DLT_EN10MB and DLT_DOCSIS, so
		 * that an application can let you choose it, in case you're
		 * capturing DOCSIS traffic that a Cisco Cable Modem
		 * Termination System is putting out onto an Ethernet (it
		 * doesn't put an Ethernet header onto the wire, it puts raw
		 * DOCSIS frames out on the wire inside the low-level
		 * Ethernet framing).
		 */
		p->dlt_list = (u_int *) malloc(sizeof(u_int) * 2);
		/*
		 * If that fails, just leave the list empty.
		 */
		if (p->dlt_list != NULL) {
			p->dlt_list[0] = DLT_EN10MB;
			p->dlt_list[1] = DLT_DOCSIS;
			p->dlt_count = 2;
		}
		break;
		
	case NdisMediumFddi:
		p->linktype = DLT_FDDI;
		break;
		
	case NdisMedium802_5:			
		p->linktype = DLT_IEEE802;	
		break;
		
	case NdisMediumArcnetRaw:
		p->linktype = DLT_ARCNET;
		break;
		
	case NdisMediumArcnet878_2:
		p->linktype = DLT_ARCNET;
		break;
		
	case NdisMediumAtm:
		p->linktype = DLT_ATM_RFC1483;
		break;
		
	case NdisMediumCHDLC:
		p->linktype = DLT_CHDLC;
		break;

	case NdisMediumPPPSerial:
		p->linktype = DLT_PPP_SERIAL;
		break;

	case NdisMediumNull:
		p->linktype = DLT_NULL;
		break;

	case NdisMediumBare80211:
		p->linktype = DLT_IEEE802_11;
		break;

	case NdisMediumRadio80211:
		p->linktype = DLT_IEEE802_11_RADIO;
		break;

	case NdisMediumPpi:
		p->linktype = DLT_PPI;
		break;

	default:
		p->linktype = DLT_EN10MB;			/*an unknown adapter is assumed to be ethernet*/
		break;
	}

	/* Set promiscuous mode */
	if (p->opt.promisc) 
	{

		if (PacketSetHwFilter(p->adapter,NDIS_PACKET_TYPE_PROMISCUOUS) == FALSE)
		{
			snprintf(p->errbuf, PCAP_ERRBUF_SIZE, "failed to set hardware filter to promiscuous mode");
			goto bad;
		}
	}
	else 
	{
		if (PacketSetHwFilter(p->adapter,NDIS_PACKET_TYPE_ALL_LOCAL) == FALSE)
		{
			snprintf(p->errbuf, PCAP_ERRBUF_SIZE, "failed to set hardware filter to non-promiscuous mode");
			goto bad;
		}
	}

	/* Set the buffer size */
	p->bufsize = WIN32_DEFAULT_USER_BUFFER_SIZE;

	/* allocate Packet structure used during the capture */
	if((p->Packet = PacketAllocatePacket())==NULL)
	{
		snprintf(p->errbuf, PCAP_ERRBUF_SIZE, "failed to allocate the PACKET structure");
		goto bad;
	}

	if(!(p->adapter->Flags & INFO_FLAG_DAG_CARD))
	{
	/* 
	 * Traditional Adapter 
	 */
		/*
		 * If the buffer size wasn't explicitly set, default to
		 * WIN32_DEFAULT_USER_BUFFER_SIZE.
		 */
	 	if (p->opt.buffer_size == 0)
	 		p->opt.buffer_size = WIN32_DEFAULT_KERNEL_BUFFER_SIZE;

		if(PacketSetBuff(p->adapter,p->opt.buffer_size)==FALSE)
		{
			snprintf(p->errbuf, PCAP_ERRBUF_SIZE, "driver error: not enough memory to allocate the kernel buffer");
			goto bad;
		}
		
		p->buffer = (u_char *)malloc(p->bufsize);
		if (p->buffer == NULL) 
		{
			snprintf(p->errbuf, PCAP_ERRBUF_SIZE, "malloc: %s", pcap_strerror(errno));
			goto bad;
		}
		
		PacketInitPacket(p->Packet,(BYTE*)p->buffer,p->bufsize);
		
		/* tell the driver to copy the buffer only if it contains at least 16K */
		if(PacketSetMinToCopy(p->adapter,16000)==FALSE)
		{
			snprintf(p->errbuf, PCAP_ERRBUF_SIZE,"Error calling PacketSetMinToCopy: %s", pcap_win32strerror());
			goto bad;
		}
	}
	else
#ifdef HAVE_DAG_API
	{
	/* 
	 * Dag Card 
	 */
		LONG	status;
		HKEY	dagkey;
		DWORD	lptype;
		DWORD	lpcbdata;
		int		postype = 0;
		char	keyname[512];
		
		snprintf(keyname, sizeof(keyname), "%s\\CardParams\\%s", 
			"SYSTEM\\CurrentControlSet\\Services\\DAG",
			strstr(_strlwr(p->opt.source), "dag"));
		do
		{
			status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyname, 0, KEY_READ, &dagkey);
			if(status != ERROR_SUCCESS)
				break;
			
			status = RegQueryValueEx(dagkey,
				"PosType",
				NULL,
				&lptype,
				(char*)&postype,
				&lpcbdata);
			
			if(status != ERROR_SUCCESS)
			{
				postype = 0;
			}
			
			RegCloseKey(dagkey);
		}
		while(FALSE);
		
		
		p->snapshot = PacketSetSnapLen(p->adapter, snaplen);
		
		/* Set the length of the FCS associated to any packet. This value 
		 * will be subtracted to the packet length */
		p->md.dag_fcs_bits = p->adapter->DagFcsLen;
	}
#else
	goto bad;
#endif /* HAVE_DAG_API */
	
	PacketSetReadTimeout(p->adapter, p->md.timeout);
	
#ifdef HAVE_DAG_API
	if(p->adapter->Flags & INFO_FLAG_DAG_CARD)
	{
		/* install dag specific handlers for read and setfilter */
		p->read_op = pcap_read_win32_dag;
		p->setfilter_op = pcap_setfilter_win32_dag;
	}
	else
	{
#endif /* HAVE_DAG_API */
		/* install traditional npf handlers for read and setfilter */
		p->read_op = pcap_read_win32_npf;
		p->setfilter_op = pcap_setfilter_win32_npf;
#ifdef HAVE_DAG_API
	}
#endif /* HAVE_DAG_API */
	p->setdirection_op = NULL;	/* Not implemented. */
	    /* XXX - can this be implemented on some versions of Windows? */
	p->inject_op = pcap_inject_win32;
	p->set_datalink_op = NULL;	/* can't change data link type */
	p->getnonblock_op = pcap_getnonblock_win32;
	p->setnonblock_op = pcap_setnonblock_win32;
	p->stats_op = pcap_stats_win32;
	p->setbuff_op = pcap_setbuff_win32;
	p->setmode_op = pcap_setmode_win32;
	p->setmintocopy_op = pcap_setmintocopy_win32;
	p->cleanup_op = pcap_cleanup_win32;

	return (0);
bad:
	pcap_cleanup_win32(p);
	return (PCAP_ERROR);
}

pcap_t *
pcap_create(const char *device, char *ebuf)
{
	pcap_t *p;

	if (strlen(device) == 1)
	{
		/*
		 * It's probably a unicode string
		 * Convert to ascii and pass it to pcap_create_common
		 *
		 * This wonderful hack is needed because pcap_lookupdev still returns
		 * unicode strings, and it's used by windump when no device is specified
		 * in the command line
		 */
		size_t length;
		char* deviceAscii;

		length = wcslen((wchar_t*)device);

		deviceAscii = (char*)malloc(length + 1);

		if (deviceAscii == NULL)
		{
			snprintf(ebuf, PCAP_ERRBUF_SIZE, "Malloc failed");
			return NULL;
		}

		snprintf(deviceAscii, length + 1, "%ws", (wchar_t*)device);
		p = pcap_create_common(deviceAscii, ebuf);
		free(deviceAscii);
	}
	else
	{
		p = pcap_create_common(device, ebuf);
	}

	if (p == NULL)
		return (NULL);

	if (IsTcDevice(p) == TRUE)
	{
		p->activate_op = TcActivate;
	}
	else
	{
	p->activate_op = pcap_activate_win32;
	}

	return (p);
}

static int
pcap_setfilter_win32_npf(pcap_t *p, struct bpf_program *fp)
{
	if(PacketSetBpf(p->adapter,fp)==FALSE){
		/*
		 * Kernel filter not installed.
		 * XXX - fall back on userland filtering, as is done
		 * on other platforms?
		 */
		snprintf(p->errbuf, PCAP_ERRBUF_SIZE, "Driver error: cannot set bpf filter: %s", pcap_win32strerror());
		return (-1);
	}

	/*
	 * Discard any previously-received packets, as they might have
	 * passed whatever filter was formerly in effect, but might
	 * not pass this filter (BIOCSETF discards packets buffered
	 * in the kernel, so you can lose packets in any case).
	 */
	p->cc = 0;
	return (0);
}

/*
 * We filter at user level, since the kernel driver does't process the packets
 */
static int 
pcap_setfilter_win32_dag(pcap_t *p, struct bpf_program *fp) {
	
	if(!fp) 
	{
		strncpy(p->errbuf, "setfilter: No filter specified", sizeof(p->errbuf));
		return -1;
	}
	
	/* Install a user level filter */
	if (install_bpf_program(p, fp) < 0) 
	{
		snprintf(p->errbuf, sizeof(p->errbuf),
			"setfilter, unable to install the filter: %s", pcap_strerror(errno));
		return -1;
	}
	
	p->md.use_bpf = 0;
	
	return (0);
}

static int
pcap_getnonblock_win32(pcap_t *p, char *errbuf)
{
	/*
	 * XXX - if there were a PacketGetReadTimeout() call, we
	 * would use it, and return 1 if the timeout is -1
	 * and 0 otherwise.
	 */
	return (p->nonblock);
}

static int
pcap_setnonblock_win32(pcap_t *p, int nonblock, char *errbuf)
{
	int newtimeout;

	if (nonblock) {
		/*
		 * Set the read timeout to -1 for non-blocking mode.
		 */
		newtimeout = -1;
	} else {
		/*
		 * Restore the timeout set when the device was opened.
		 * (Note that this may be -1, in which case we're not
		 * really leaving non-blocking mode.)
		 */
		newtimeout = p->md.timeout;
	}
	if (!PacketSetReadTimeout(p->adapter, newtimeout)) {
		snprintf(p->errbuf, PCAP_ERRBUF_SIZE,
		    "PacketSetReadTimeout: %s", pcap_win32strerror());
		return (-1);
	}
	p->nonblock = (newtimeout == -1);
	return (0);
}

/*platform-dependent routine to add devices other than NDIS interfaces*/
int
pcap_platform_finddevs(pcap_if_t **alldevsp, char *errbuf)
{
	return TcFindAllDevs(alldevsp, errbuf);
}
