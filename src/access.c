/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* All files in http2d are Copyright (C) 2012 Alvaro Lopez Ortega.
 *
 *   Authors:
 *     * Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
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
 */

#include "common-internal.h"
#include "access.h"

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#include <limits.h>

#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif

#include "util.h"
#include "resolv_cache.h"
#include "error_log.h"

#define ENTRIES "access"


#ifndef AF_INET6
# define AF_INET6 10
#endif

typedef enum {
	ipv4 = AF_INET,
	ipv6 = AF_INET6
} ip_type_t;


typedef union {
	struct in_addr  ip4;
#ifdef HAVE_IPV6
	struct in6_addr ip6;
#endif
} ip_t;

typedef struct {
	http2d_list_t node;

	ip_type_t       type;
	ip_t            ip;
} ip_item_t;

typedef struct {
	ip_item_t base;
	ip_t      mask;
} subnet_item_t;

#define IP_NODE(x)     ((ip_item_t *)(x))
#define SUBNET_NODE(x) ((subnet_item_t *)(x))


static ip_item_t *
new_ip (void)
{
	ip_item_t *n = (ip_item_t *) malloc (sizeof(ip_item_t));
	if (n == NULL) return NULL;

	INIT_LIST_HEAD (LIST(n));
	memset (&n->ip, 0, sizeof(ip_t));

	return n;
}

static ret_t
free_ip (ip_item_t *ip)
{
	free (ip);
	return ret_ok;
}

static subnet_item_t *
new_subnet (void)
{
	subnet_item_t *n = (subnet_item_t *) malloc (sizeof(subnet_item_t));
	if (n == NULL) return NULL;

	memset (&n->base.ip, 0, sizeof(ip_t));
	memset (&n->mask, 0, sizeof(ip_t));

	INIT_LIST_HEAD (LIST(n));
	return n;
}


ret_t
http2d_access_init (http2d_access_t *entry)
{
	INIT_LIST_HEAD(&entry->list_ips);
	INIT_LIST_HEAD(&entry->list_subnets);
	return ret_ok;
}

ret_t
http2d_access_mrproper (http2d_access_t *entry)
{
	http2d_list_t *i, *tmp;

	/* Free the IP list items
	 */
	list_for_each_safe (i, tmp, LIST(&entry->list_ips)) {
		http2d_list_del (i);
		free (i);
	}

	/* Free the Subnet list items
	 */
	list_for_each_safe (i, tmp, LIST(&entry->list_subnets)) {
		http2d_list_del (i);
		free (i);
	}

	return ret_ok;
}

HTTP2D_ADD_FUNC_NEW (access);
HTTP2D_ADD_FUNC_FREE (access);


static void
print_ip (ip_type_t type, ip_t *ip)
{
	HTTP2D_TEMP_VARS(dir,255);

#ifdef HAVE_INET_PTON
# ifdef HAVE_IPV6
	if (type == ipv6) {
		printf ("%s", inet_ntop (AF_INET6, ip, dir, dir_size));
		return;
	}
# endif
	if (type == ipv4) {
		printf ("%s", inet_ntop (AF_INET, ip, dir, dir_size));
		return;
	}
#else
	printf ("%s", inet_ntoa (ip->ip4));
#endif
}


static ret_t
parse_ip (char *ip, ip_item_t *n)
{
	int ok;

	/* Test if it is a IPv4 or IPv6 connection
	 */
	n->type = ((strchr (ip, ':') != NULL) ||
		   (strlen(ip) > 15)) ? ipv6 : ipv4;

#ifndef HAVE_IPV6
	if (n->type == ipv6) {
		return ret_error;
	}
#endif

	/* Parse the IP string
	 */
#ifdef HAVE_INET_PTON
	ok = (inet_pton (n->type, ip, &n->ip) > 0);

# ifdef HAVE_IPV6
	if (n->type == ipv6) {
		if (IN6_IS_ADDR_V4MAPPED (&(n->ip).ip6)) {
			LOG_ERROR (HTTP2D_ERROR_ACCESS_IPV4_MAPPED, ip);
			return ret_error;
		}
	}
# endif /* HAVE_IPV6 */

#else
	ok = (inet_aton (ip, (struct in_addr *)&n->ip) != 0);
#endif

	return (ok) ? ret_ok : ret_error;
}


static ret_t
parse_netmask (char *netmask, subnet_item_t *n)
{
	int num;

	/* IPv6 or IPv4 Mask
	 * Eg: 255.255.0.0
	 */
	if ((strchr (netmask, ':') != NULL) ||
	    (strchr (netmask, '.') != NULL))
	{
		int ok;

#ifdef HAVE_INET_PTON
		ok = (inet_pton (IP_NODE(n)->type, netmask, &n->mask) > 0);
#else
		ok = (inet_aton (netmask, (struct in_addr *)&n->mask) != 0);
#endif
		return (ok) ? ret_ok : ret_error;
	}


	/* Length mask
	 * Eg: 16
	 */
	if (strlen(netmask) > 3) {
		return ret_error;
	}

	num = strtol(netmask, NULL, 10);
	if (num < 0)
		return ret_error;

	/* Sanity checks
	 */
	if ((IP_NODE(n)->type == ipv4) && (num >  32))
		return ret_error;

	if ((IP_NODE(n)->type == ipv6) && (num > 128))
		return ret_error;

#ifdef HAVE_IPV6
	if (num > 128) {
		return ret_error;
	}

	/* Special case
	 */
	if (num == 128) {
		int i;

		for (i=0; i<16; i++) {
			n->mask.ip6.s6_addr[i] = 0;
		}

		return ret_ok;
	}

	if (IP_NODE(n)->type == ipv6) {
		int i, j, jj;
		unsigned char mask    = 0;
		unsigned char maskbit = 0x80L;

		j  = (int) num / 8;
		jj = num % 8;

		for (i=0; i<j; i++) {
			n->mask.ip6.s6_addr[i] = 0xFF;
		}

		while (jj--) {
			mask |= maskbit;
			maskbit >>= 1;
		}
		n->mask.ip6.s6_addr[j] = mask;

		return ret_ok;
	} else
#endif
		if (num == 0)
			n->mask.ip4.s_addr = (in_addr_t) 0;
		else
			n->mask.ip4.s_addr = (in_addr_t) htonl(~0L << (32 - num));

	return ret_ok;
}


static ret_t
http2d_access_add_ip (http2d_access_t *entry, char *ip)
{
	ret_t      ret;
	ip_item_t *n;

	n = new_ip();
	if (n == NULL) return ret_error;

	ret = parse_ip (ip, n);
	if (ret < ret_ok) {
		LOG_ERROR (HTTP2D_ERROR_ACCESS_INVALID_IP, ip);

		free_ip(n);
		return ret;
	}

	http2d_list_add (LIST(n), &entry->list_ips);
	TRACE (ENTRIES, "Access: adding IP '%s'\n", ip);

	return ret;
}


static ret_t
http2d_access_add_domain (http2d_access_t *entry, char *domain)
{
	ret_t                    ret;
	char                     ip[46]; // Max IPv6 length is 45
	http2d_resolv_cache_t *resolv;
	const struct addrinfo   *addr_info, *addr;
	http2d_buffer_t        domain_buf = HTTP2D_BUF_INIT;

	http2d_buffer_fake (&domain_buf, domain, strlen(domain));

	ret = http2d_resolv_cache_get_default (&resolv);
	if (unlikely(ret!=ret_ok)) return ret;

	ret = http2d_resolv_cache_get_addrinfo (resolv, &domain_buf, &addr_info);
	if (unlikely(ret!=ret_ok)) return ret;

	addr = addr_info;
	while (addr != NULL) {
		ret = http2d_ntop (addr->ai_family, addr->ai_addr, ip, sizeof(ip));
		if (unlikely(ret!=ret_ok)) return ret;

		TRACE (ENTRIES, "Access: domain '%s' -> IP: %s\n", domain, ip);
		ret = http2d_access_add_ip (entry, (char *)ip);
		if (unlikely(ret!=ret_ok)) return ret;

		addr = addr->ai_next;
	}

	return ret_ok;
}


static ret_t
http2d_access_add_subnet (http2d_access_t *entry, char *subnet)
{
	ret_t              ret;
	char              *slash;
	char              *mask;
	subnet_item_t     *n;
	http2d_buffer_t  ip = HTTP2D_BUF_INIT;

	/* Split the string
	 */
	slash = strpbrk (subnet, "/\\");
	if (slash == NULL) return ret_error;

	mask = slash +1;
	http2d_buffer_add (&ip, subnet, mask-subnet-1);

	/* Create the new list object
	 */
	n = new_subnet();
	if (n == NULL) return ret_error;

	http2d_list_add (LIST(n), &entry->list_subnets);

	/* Parse the IP
	 */
	ret = parse_ip (ip.buf, IP_NODE(n));
	if (ret < ret_ok) {
		LOG_ERROR (HTTP2D_ERROR_ACCESS_INVALID_IP, ip.buf);
		goto error;
	}

	/* Parse the Netmask
	 */
	ret = parse_netmask (mask, n);
	if (ret < ret_ok) {
		LOG_ERROR (HTTP2D_ERROR_ACCESS_INVALID_MASK, mask);
		goto error;
	}

	TRACE (ENTRIES, "Access: subnet IP '%s', mask '%s'\n", ip.buf, mask);

	http2d_buffer_mrproper (&ip);
	return ret_ok;

error:
	http2d_buffer_mrproper (&ip);
	return ret_error;
}


ret_t
http2d_access_add (http2d_access_t *entry, char *ip_or_subnet)
{
	ret_t  ret;
	char  *slash;
	int    mask;
	char   sep;
	int    colon;

	slash = strpbrk (ip_or_subnet, "/\\");
	colon = (strchr (ip_or_subnet, ':') != NULL);

	/* Add a single IP address
	 */
	if (slash == NULL) {
		char *i         = ip_or_subnet;
		int   is_domain = 0;

		if (! colon) {
			while (*i && !is_domain) {
				if (((*i >= 'a') && (*i <= 'z')) ||
				    ((*i >= 'A') && (*i <= 'Z'))) {
					is_domain = 1;
				}
				i++;
			}
		}

		if (is_domain)
			return http2d_access_add_domain (entry, ip_or_subnet);
		else
			return http2d_access_add_ip (entry, ip_or_subnet);
	}

	/* Special cases of subnets
	 */
	ret = http2d_atoi (slash+1, &mask);
	if (unlikely (ret != ret_ok))
		return ret_error;

	if (colon && (mask == 128)) {
		sep = *slash;
		*slash = '\0';
		ret = http2d_access_add_ip (entry, ip_or_subnet);
		*slash = sep;
		return ret;
	}

	if ((strchr(ip_or_subnet, '.') != NULL) && (mask == 32)) {
		sep = *slash;
		*slash = '\0';
		ret = http2d_access_add_ip (entry, ip_or_subnet);
		*slash = sep;
		return ret;
	}

	/* Add a subnet
	 */
	return http2d_access_add_subnet (entry, ip_or_subnet);
}


ret_t
http2d_access_print_debug (http2d_access_t *entry)
{
	http2d_list_t *i;

	printf ("IPs: ");
	list_for_each (i, LIST(&entry->list_ips)) {
		print_ip (IP_NODE(i)->type, &IP_NODE(i)->ip);
		printf(" ");
	}
	printf("\n");

	printf ("Subnets: ");
	list_for_each (i, LIST(&entry->list_subnets)) {
		print_ip (IP_NODE(i)->type, &IP_NODE(i)->ip);
		printf("/");
		print_ip (IP_NODE(i)->type, &SUBNET_NODE(i)->mask);
		printf(" ");
	}
	printf("\n");

	return ret_ok;
}


ret_t
http2d_access_ip_match (http2d_access_t *entry, http2d_socket_t *sock)
{
	int              re;
	http2d_list_t *i;

	TRACE (ENTRIES, "Matching ip(%x)\n", SOCKET_ADDR_IPv4(sock)->sin_addr);

	/* Check in the IP list
	 */
	list_for_each (i, LIST(&entry->list_ips)) {

#ifdef HAVE_IPV6
		/* This is a special case:
		 * The socket is IPv6 with a mapped IPv4 address
		 */
		if ((SOCKET_AF(sock) == ipv6) &&
		    (IP_NODE(i)->type == ipv4) &&
		    (IN6_IS_ADDR_V4MAPPED (&SOCKET_ADDR_IPv6(sock)->sin6_addr)) &&
		    (!memcmp (&SOCKET_ADDR_IPv6(sock)->sin6_addr.s6_addr[12], &IP_NODE(i)->ip, 4)))
		{
			TRACE (ENTRIES, "IPv4 mapped in IPv6 address: %s\n", "matched");
			return ret_ok;
		}
#endif

		if (SOCKET_AF(sock) == IP_NODE(i)->type) {
			switch (IP_NODE(i)->type) {
			case ipv4:
				re = memcmp (&SOCKET_ADDR_IPv4(sock)->sin_addr, &IP_NODE(i)->ip, 4);
				/*
				printf ("4 remote "); print_ip(ipv4, &SOCKET_ADDRESS_IPv4(sock)); printf ("\n");
				printf ("4 list   "); print_ip(ipv4, &IP_NODE(i)->ip); printf ("\n");
				*/
				TRACE (ENTRIES, "IPv4 address (%x)%s matched (ip=%x)\n",
				       IP_NODE(i)->ip.ip4,
				       re ? " haven't" : "",
				       SOCKET_ADDR_IPv4(sock)->sin_addr);
				break;
#ifdef HAVE_IPV6
			case ipv6:
				re = (! IN6_ARE_ADDR_EQUAL (&SOCKET_ADDR_IPv6(sock)->sin6_addr, &IP_NODE(i)->ip.ip6));

				/* re = memcmp (&SOCKET_ADDR_IPv6(sock)->sin6_addr, &IP_NODE(i)->ip, 16); */

				/*
				printf ("6 family=%d, ipv6=%d\n", SOCKET_ADDR_IPv6(sock)->sin6_family, ipv6);
				printf ("6 port=%d\n",            SOCKET_ADDR_IPv6(sock)->sin6_port);
				printf ("6 remote "); print_ip(ipv6, &SOCKET_ADDRESS_IPv6(sock)); printf ("\n");
				printf ("6 list   "); print_ip(ipv6, &IP_NODE(i)->ip); printf ("\n");
				printf ("6 re = %d\n", re);
				*/
				break;
#endif
			default:
				SHOULDNT_HAPPEN;
				return ret_error;
			}

			if (re == 0) {
				return ret_ok;
			}
		}
	}


	/* Check in the Subnets list
	 */
	list_for_each (i, LIST(&entry->list_subnets)) {
		int j;
		ip_t masqued_remote, masqued_list;

#ifdef HAVE_IPV6
		if ((SOCKET_AF(sock) == ipv6) &&
		    (IP_NODE(i)->type == ipv4) &&
		    IN6_IS_ADDR_V4MAPPED (&SOCKET_ADDR_IPv6(sock)->sin6_addr))
		{
			cuint_t ip4_sock;

			memcpy (&ip4_sock, &SOCKET_ADDR_IPv6(sock)->sin6_addr.s6_addr[12], 4);

			masqued_list.ip4.s_addr   = (IP_NODE(i)->ip.ip4.s_addr &
						     SUBNET_NODE(i)->mask.ip4.s_addr);
			masqued_remote.ip4.s_addr = (ip4_sock &
						     SUBNET_NODE(i)->mask.ip4.s_addr);

			if (masqued_remote.ip4.s_addr == masqued_list.ip4.s_addr) {
				return ret_ok;
			}
		}
#endif

		if (SOCKET_AF(sock) == IP_NODE(i)->type) {
			switch (IP_NODE(i)->type) {
			case ipv4:
				masqued_list.ip4.s_addr   = (IP_NODE(i)->ip.ip4.s_addr &
							     SUBNET_NODE(i)->mask.ip4.s_addr);
				masqued_remote.ip4.s_addr = (SOCKET_ADDR_IPv4(sock)->sin_addr.s_addr &
							     SUBNET_NODE(i)->mask.ip4.s_addr);

				TRACE (ENTRIES, "Checking IPv4 net: (mask=%x) %x == %x ?\n",
				       SUBNET_NODE(i)->mask.ip4.s_addr,
				       masqued_remote.ip4.s_addr,
				       masqued_list.ip4.s_addr);

				if (masqued_remote.ip4.s_addr == masqued_list.ip4.s_addr) {
					return ret_ok;
				}

				break;
#ifdef HAVE_IPV6
			case ipv6:
			{
				bool equal = true;

				for (j=0; j<16; j++) {
					masqued_list.ip6.s6_addr[j] = (
						IP_NODE(i)->ip.ip6.s6_addr[j] &
						SUBNET_NODE(i)->mask.ip6.s6_addr[j]);
					masqued_remote.ip6.s6_addr[j] = (
						SOCKET_ADDR_IPv6(sock)->sin6_addr.s6_addr[j] &
						SUBNET_NODE(i)->mask.ip6.s6_addr[j]);

					if (masqued_list.ip6.s6_addr[j] !=
					    masqued_remote.ip6.s6_addr[j])
					{
						equal = false;
						break;
					}
				}

				TRACE (ENTRIES, "Checking IPv6 net: (mask=%x) %x == %x ?\n",
				       SUBNET_NODE(i)->mask.ip6.s6_addr,
				       masqued_remote.ip6.s6_addr,
				       masqued_list.ip6.s6_addr);


				if (equal == true) {
					return ret_ok;
				}
				break;
			}
#endif
			default:
				SHOULDNT_HAPPEN;
				return ret_error;
			}
		}
	}

	return ret_not_found;
}
