/*
 *  dj64 - 64bit djgpp-compatible tool-chain
 *  Copyright (C) 2021-2025  @stsp
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* minimal getaddrinfo() */

int getaddrinfo(const char *restrict host, const char *restrict serv,
    const struct addrinfo *restrict hint, struct addrinfo **restrict res)
{
	struct addrinfo *ret = malloc(sizeof(*ret));
	struct sockaddr_in *sin = (struct sockaddr_in *)ret->ai_addr;
	struct hostent *h = gethostbyname(host);

	if (!h)
		return -1;
	ret->ai_flags = 0;
	ret->ai_family = AF_INET;
	ret->ai_socktype = SOCK_STREAM;
	ret->ai_protocol = IPPROTO_TCP;
	ret->ai_addrlen = sizeof(struct sockaddr_in);
	sin->sin_addr.s_addr = inet_addr(h->h_addr);
	ret->ai_canonname = strdup(h->h_name);
	ret->ai_next = NULL;
	*res = ret;
	return 0;
}

void freeaddrinfo(struct addrinfo *res)
{
	free(res->ai_canonname);
	free(res);
}
