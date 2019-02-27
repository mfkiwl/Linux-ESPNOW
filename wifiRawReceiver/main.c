/*
Modified by FCaminade

Sources : 
https://stackoverflow.com/questions/10824827/raw-sockets-communication-over-wifi-receiver-not-able-to-receive-packets

1/Find your wifi interface:
$ iwconfig

2/Setup the bord in monitor mode :
$ sudo ifconfig wlp5s0 down
$ sudo iwconfig wlp5s0 mode monitor
$ sudo ifconfig wlp5s0 up

3/Launch the test with the good rights! :
$ ./test3 wlp5s0 or sudo ./test3 wlp5s0

*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/sockios.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <linux/if.h>
#include <linux/if_arp.h>
#include <arpa/inet.h>
#include <assert.h>

#define PACKET_LENGTH 400 //Approximate
#define MYDATA 18         //0x12

/*our MAC address*/
//{0xF8, 0x1A, 0x67, 0xB7, 0xeB, 0x0B};

/*ESP8266 host MAC address*/
//{0x84,0xF3,0xEB,0x73,0x55,0x0D};

//Exemple of ESPNOW packet :
/*
{0x00,0x00,0xd0,0x00,0x3c,0x00,0x84,0xf3,0xeb,0x73,0x55,0x0d,0x86,0xf3,0xeb,0x73,
0xca,0x61,0x84,0xf3,0xeb,0x73,0x55,0x0d,0xa0,0x09,0x7f,0x18,0xfe,0x34,0x17,0x71,
0x47,0x8c,0xdd,0xff,0x18,0xfe,0x34,0x04,0x01,0x62,0x12,0x12,0x12,0x12,0x12,0x12,
0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,
0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,
0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,
0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,
0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,
0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,
0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,
0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,
0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,
0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,
0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,
0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,
0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,
0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,
0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,
0x12,0x12,0x12,0x1d,0x2c,0xba,0x98}
*/

struct newpacket
{
    unsigned char newdata[PACKET_LENGTH];
} mynewpacket;

void printfstruct()
{
    printf("----------------------------new packet-----------------------------------\n");

    if (MYDATA == mynewpacket.newdata[197]) //Filtering espnow packets
    {
        if (MYDATA == mynewpacket.newdata[200]) //Filtering espnow packets
        {
            for (int i = 0; i < PACKET_LENGTH; i++)
            {
                if (i % 16 == 0)
                    printf("\n");
                printf("%02x ", mynewpacket.newdata[i]);
            }
        }
    }

    printf("\n\n");
}

int create_raw_socket(char *dev)
{
    struct sockaddr_ll sll;
    struct ifreq ifr;
    int fd, ifi, rb;

    bzero(&sll, sizeof(sll));
    bzero(&ifr, sizeof(ifr));

    fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    assert(fd != -1);

    strncpy((char *)ifr.ifr_name, dev, IFNAMSIZ);
    ifi = ioctl(fd, SIOCGIFINDEX, &ifr);
    assert(ifi != -1);
    sll.sll_protocol = htons(ETH_P_ALL);
    sll.sll_family = PF_PACKET;
    sll.sll_ifindex = ifr.ifr_ifindex;
    sll.sll_pkttype = PACKET_OTHERHOST;
    rb = bind(fd, (struct sockaddr *)&sll, sizeof(sll));
    assert(rb != -1);

    return fd;
}

int main(int argc, char **argv)
{
    int sock_fd;
    char *dev = argv[1];

    sock_fd = create_raw_socket(dev); /* Creating the raw socket */

    printf("\n Waiting to receive packets ........ \n");

    while (1)
    {
        struct sockaddr_ll s_sender_addr;
        socklen_t u32_sender_addr_len = sizeof(s_sender_addr);
        (void)memset(&s_sender_addr, 0, sizeof(s_sender_addr));

        int x = recvfrom(sock_fd, &mynewpacket, sizeof(struct newpacket), 0, (struct sockaddr *)&s_sender_addr, &u32_sender_addr_len);

        if (-1 == x)
        {
            perror("Socket receive failed");
            break;
        }
        else if (x < 0)
        {
            perror("Socket receive, error ");
        }

        printfstruct();
    }
    close(sock_fd);
    return 0;
}