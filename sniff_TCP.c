#include <stdio.h>
#include <stdlib.h>
#include <pcap.h>
#include <arpa/inet.h>

int cnt;                                                                                                   // 몇 번째 패킷인지 출력하는 전역변수 cnt

struct ethheader {                                                                                         // Ethernet 헤더
	u_char ether_dhost[6];                                                                                   // 목적지 MAC 주소
	u_char ether_shost[6];                                                                                   // 출발지 MAC 주소
	u_short ether_type;                                                                                      // 타입(상위 프로토콜 종류)
};

struct ipheader {                                                                                          // IP 프로토콜 헤더
	unsigned char      iph_ihl:4, iph_ver:4;                                                                 // 헤더 길이(기본값 4), IP 버전(기본값 4)
	unsigned char 	   iph_tos;                                                                              // Type of Service
	unsigned short int iph_len;                                                                              // 전체 패킷 길이
	unsigned short int iph_ident;                                                                            // 파편화되었을 때 식별번호
	unsigned short int iph_flag:3, iph_offset:13;                                                            // 플래그(기본값 3), 오프셋(기본값 13)
	unsigned char      iph_ttl;                                                                              // Time to Live(패킷의 수명)
	unsigned char      iph_protocol;                                                                         // 상위 프로토콜 종류
	struct in_addr     iph_sourceip;                                                                         // 출발지 IP 주소
	struct in_addr     iph_destip;                                                                           // 목적지 IP 주소
};

struct tcpheader {                                                                                         // TCP 프로토콜 헤더
	u_short tcp_sport;                                                                                       // 출발지 포트번호
	u_short tcp_dport;                                                                                       // 목적지 포트번호
	u_int   tcp_seq;                                                                                         // 세그먼트 일련번호 (전송하는 데이터의 순서를 나타냄)
	u_int   tcp_ack;                                                                                         // 세그먼트 확인번호 (다음으로 기대하는 세그먼트의 번호)
	u_char  tcp_offx2;                                                                                       // 세그먼트에서 실제 데이터가 시작하는 위치(4바이트 단위)
	#define TH_OFF(th)      (((th)->tcp_offx2 & 0xf0) >> 4)                                                  // 실제 데이터가 시작하는 지점을 알아내는 매크로
	u_char  tcp_flags;                                                                                       // TCP 플래그
	#define TH_FIN  0x01                                                                                     // FIN 플래그
	#define TH_SYN  0x02                                                                                     // SYN 플래그
	#define TH_RST  0x04                                                                                     // RST 플래그
	#define TH_PUSH 0x08                                                                                     // PSH 플래그
	#define TH_ACK  0x10                                                                                     // ACK 플래그
	#define TH_URG  0x20                                                                                     // URG 플래그
	#define TH_ECE  0x40                                                                                     // ECE 플래그
	#define TH_CWR  0x80                                                                                     // CWR 플래그
	#define TH_FLAGS        (TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG|TH_ECE|TH_CWR)               
    	u_short tcp_win;                                                                                     // 윈도우 크기(한 번에 보낼 수 있는 데이터 크기)
    	u_short tcp_sum;                                                                                     // 체크섬
    	u_short tcp_urp;                                                                                     // 긴급 포인터(URG 플래그가 설정되어 있으면 이 포인터가 가리키는 위치를 우선적으로 처리)
};
	
void got_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet) {                    // 패킷을 캡처했을 때 호출되는 함수
	struct ethheader *eth = (struct ethheader *)packet;                                                      // 패킷의 시작점부터 이더넷 헤더 타입으로 참조
	
	if (ntohs(eth->ether_type) == 0x0800) {                                                                  // 이더넷 헤더의 타입이 0x0800(IP)인 경우에만 처리
		struct ipheader *ip = (struct ipheader *)(packet + sizeof(struct ethheader));                          // 패킷의 시작점부터 이더넷 헤더 크기만큼 떨어진 위치부터 IP 헤더 시작
		if (ip->iph_protocol != IPPROTO_TCP) return;                                                           // IP 상위 프로토콜이 TCP가 아닌 경우 무시
		printf("================[Headers %04d]====================\n", cnt);
		int i;
		
		printf("%10s : %02x", "src MAC", eth->ether_dhost[0]);                                                 // 출발지 MAC 주소 출력
		for(i=1;i<6;i++) printf(":%02x", eth->ether_dhost[i]);
		
		printf("\n%10s : %02x", "dst MAC", eth->ether_shost[0]);                                               // 목적지 MAC 주소 출력
		for(i=1;i<6;i++) printf(":%02x", eth->ether_shost[i]);
		
		printf("\n%10s : %s\n", "src IP", inet_ntoa(ip->iph_sourceip));                                        // 출발지 IP 주소 출력
		printf("%10s : %s\n", "dst IP", inet_ntoa(ip->iph_destip));                                            // 목적지 IP 주소 출력
		
		int ip_header_len = ip->iph_ihl * 4;                                                                   // IP 헤더 길이 계산
		struct tcpheader *tcp = (struct tcpheader *)(packet + sizeof(struct ethheader) + ip_header_len);       // IP 헤더 길이를 이용하여 TCP 헤더 시작 위치 계산
		
		printf("%10s : %u\n", "src PORT", ntohs(tcp->tcp_sport));                                              // 출발지 포트 번호 출력
		printf("%10s : %u\n", "dst PORT", ntohs(tcp->tcp_dport));                                              // 목적지 포트 번호 출력
		
		printf("%10s : ", "flags");                                                                            // 플래그 값 참고하여 활성화된 플래그 출력
		char *flags[] = {"FIN", "SYN", "RST", "PSH", "ACK", "URG", "ECE", "CWR"};
		for(i=0;i<8;i++) {
			if (tcp->tcp_flags & (1 << i)) printf("%s ", flags[i]);
		}
		              
		printf("\n================[Message %04d]====================\n\n", cnt++);
		
		int tcp_header_len = TH_OFF(tcp)*4;                                                                    // TCP의 데이터 오프셋을 이용하여 TCP 헤더 길이 계산
		
		u_char *payload = (u_char *)(packet+sizeof(struct ethheader) + ip_header_len + tcp_header_len);        // TCP 헤더 길이를 이용하여 payload 시작 지점 계산
		int payload_len = ntohs(ip->iph_len) - (ip_header_len + tcp_header_len);                               // payload 길이 계산 (IP 패킷 전체 길이 - IP 헤더 길이 - TCP 헤더 길이)
		if (payload_len==0) printf("No payload\n\n");                                                          // 길이가 0이면 payload 없음
		else {                                                                                                 // 있으면 출력
			for(int i=1;i<=payload_len;i++) printf("%c", payload[i-1]);
			printf("\n\n");
		}
  	}	
}

int main() {
	pcap_t *handle;                                                                                           // pcap 핸들러 변수 handle
	char errbuf[PCAP_ERRBUF_SIZE];                                                                            // 에러 메시지 저장
	struct bpf_program fp;                                                                                    // 필터 표현식 컴파일 결과 저장
	char filter_exp[] = "tcp";                                                                                // 필터 표현식 = tcp(TCP만 오게 필터링)
	bpf_u_int32 net;
	
	handle = pcap_open_live("eth0", BUFSIZ, 1, 1000, errbuf);                                                 // "eth0" 네트워크 인터페이스를 사용하여 패킷 캡처 시작
	pcap_compile(handle, &fp, filter_exp, 0, net);                                                            // 필터 표현식 컴파일하여 BPF 프로그램으로 변환
	if (pcap_setfilter(handle, &fp) != 0) {                                                                   // 컴파일된 필터를 handle에 설정
		pcap_perror(handle, "Error");
		exit(EXIT_FAILURE);
	}
	
	pcap_loop(handle, -1, got_packet, NULL);                                                                  // 패킷 캡처 루프 시작(-1은 무한 루프를 의미하며, 패킷이 캡처될 때마다 got_packet 함수 호출)
	pcap_close(handle);                                                                                       // pcap 핸들 종료
	return 0;
}
