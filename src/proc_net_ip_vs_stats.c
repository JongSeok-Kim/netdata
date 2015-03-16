#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "procfile.h"
#include "rrd.h"
#include "plugin_proc.h"

#define RRD_TYPE_NET_IPVS 			"ipvs"
#define RRD_TYPE_NET_IPVS_LEN		strlen(RRD_TYPE_NET_IPVS)

int do_proc_net_ip_vs_stats(int update_every, unsigned long long dt) {
	static int do_bandwidth = -1, do_sockets = -1, do_packets = -1;
	static procfile *ff = NULL;

	if(do_bandwidth == -1)	do_bandwidth 	= config_get_boolean("plugin:proc:/proc/net/ip_vs_stats", "IPVS bandwidth", 1);
	if(do_sockets == -1)	do_sockets 		= config_get_boolean("plugin:proc:/proc/net/ip_vs_stats", "IPVS connections", 1);
	if(do_packets == -1)	do_packets 		= config_get_boolean("plugin:proc:/proc/net/ip_vs_stats", "IPVS packets", 1);

	if(dt) {};

	if(!ff) ff = procfile_open("/proc/net/ip_vs_stats", " \t,:|");
	if(!ff) return 1;

	ff = procfile_readall(ff);
	if(!ff) return 0; // we return 0, so that we will retry to open it next time

	// make sure we have 3 lines
	if(procfile_lines(ff) < 3) return 1;

	// make sure we have 5 words on the 3rd line
	if(procfile_linewords(ff, 2) < 5) return 1;

	unsigned long long entries, InPackets, OutPackets, InBytes, OutBytes;

	entries 	= strtoull(procfile_lineword(ff, 2, 0), NULL, 10);
	InPackets 	= strtoull(procfile_lineword(ff, 2, 1), NULL, 10);
	OutPackets 	= strtoull(procfile_lineword(ff, 2, 2), NULL, 10);
	InBytes 	= strtoull(procfile_lineword(ff, 2, 3), NULL, 10);
	OutBytes 	= strtoull(procfile_lineword(ff, 2, 4), NULL, 10);

	RRD_STATS *st;

	// --------------------------------------------------------------------

	if(do_sockets) {
		st = rrd_stats_find(RRD_TYPE_NET_IPVS ".sockets");
		if(!st) {
			st = rrd_stats_create(RRD_TYPE_NET_IPVS, "sockets", NULL, RRD_TYPE_NET_IPVS, "IPVS New Connections", "connections/s", 1001, update_every, CHART_TYPE_LINE);

			rrd_stats_dimension_add(st, "connections", NULL, 1, 1 * update_every, RRD_DIMENSION_INCREMENTAL);
		}
		else rrd_stats_next(st);

		rrd_stats_dimension_set(st, "connections", entries);
		rrd_stats_done(st);
	}

	// --------------------------------------------------------------------
	
	if(do_packets) {
		st = rrd_stats_find(RRD_TYPE_NET_IPVS ".packets");
		if(!st) {
			st = rrd_stats_create(RRD_TYPE_NET_IPVS, "packets", NULL, RRD_TYPE_NET_IPVS, "IPVS Packets", "packets/s", 1002, update_every, CHART_TYPE_LINE);

			rrd_stats_dimension_add(st, "received", NULL, 1, 1 * update_every, RRD_DIMENSION_INCREMENTAL);
			rrd_stats_dimension_add(st, "sent", NULL, -1, 1 * update_every, RRD_DIMENSION_INCREMENTAL);
		}
		else rrd_stats_next(st);

		rrd_stats_dimension_set(st, "received", InPackets);
		rrd_stats_dimension_set(st, "sent", OutPackets);
		rrd_stats_done(st);
	}

	// --------------------------------------------------------------------
	
	if(do_bandwidth) {
		st = rrd_stats_find(RRD_TYPE_NET_IPVS ".net");
		if(!st) {
			st = rrd_stats_create(RRD_TYPE_NET_IPVS, "net", NULL, RRD_TYPE_NET_IPVS, "IPVS Bandwidth", "kilobits/s", 1000, update_every, CHART_TYPE_AREA);

			rrd_stats_dimension_add(st, "received", NULL, 8, 1024 * update_every, RRD_DIMENSION_INCREMENTAL);
			rrd_stats_dimension_add(st, "sent", NULL, -8, 1024 * update_every, RRD_DIMENSION_INCREMENTAL);
		}
		else rrd_stats_next(st);

		rrd_stats_dimension_set(st, "received", InBytes);
		rrd_stats_dimension_set(st, "sent", OutBytes);
		rrd_stats_done(st);
	}

	return 0;
}