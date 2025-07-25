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

int getaddrinfo(const char *host, const char *serv,
    const struct addrinfo *hint, struct addrinfo **res)
{
	struct addrinfo *ret;
	struct sockaddr_in *sin;
	struct hostent *h = gethostbyname(host);

	if (!h || !h->h_addr)
		return -1;
	ret = malloc(sizeof(*ret));
	sin = malloc(sizeof(*sin));
	ret->ai_flags = 0;
	ret->ai_family = AF_INET;
	ret->ai_socktype = SOCK_STREAM;
	ret->ai_protocol = IPPROTO_TCP;
	ret->ai_addrlen = sizeof(struct sockaddr_in);
	sin->sin_addr = *(struct in_addr *)h->h_addr;
	ret->ai_addr = (struct sockaddr *)sin;
	ret->ai_canonname = strdup(h->h_name);
	ret->ai_next = NULL;
	*res = ret;
	return 0;
}

void freeaddrinfo(struct addrinfo *res)
{
	free(res->ai_canonname);
	free(res->ai_addr);
	free(res);
}
