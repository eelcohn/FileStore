/* client.h
 * Client application for testing AUN (Econet over IP) based connections
 *
 * (c) Eelco Huininga 2017-2018
 */

const char *helpstring = "\
Usage: client [OPTION...] [PARAMETER]...\n\
Client application for testing AUN (Econet over IP) based connections.\n\
\n\
Examples:\n\
  client --host 127.0.0.1 --file econet.bin   # Transmit the contents of\n\
                                                econet.bin to 127.0.0.1\n\
  client --host acorn.co.uk --hex ffff0010    # Transmit 4 bytes (ff:ff:00:10)\n\
                                                to acorn.co.uk.\n\
\n\
 Parameters:\n\
\n\
  -f, --file <filename>             transmit contents of file\n\
  -h, --host <ip|hostname(:port)>   specify ip or hostname to connect to\n\
  -x, --hex                         transmit hex bytes\n\
  -s, --string                      transmit ASCII string\n\
  -d, --dtls                        encrypt the connection using DTLS\n\
  -4, --ipv4                        use ipv4\n\
  -6, --ipv6                        use ipv6\n\
      --help                        show help message\n\
";

void dtls_connect(void);
void udp_connect(char *address, unsigned short port);

