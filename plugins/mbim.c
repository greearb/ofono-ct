/*
 *
 *  oFono - Open Source Telephony
 *
 *  Copyright (C) 2017  Intel Corporation. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <linux/types.h>
#include <unistd.h>
#include <stdlib.h>

#define OFONO_API_SUBJECT_TO_CHANGE
#include <ofono/plugin.h>
#include <ofono/modem.h>
#include <ofono/log.h>

#include <ell/ell.h>

#include <drivers/mbimmodem/mbim-desc.h>
#include <drivers/mbimmodem/util.h>

struct mbim_data {
	struct mbim_device *device;
	uint16_t max_segment;
	uint8_t max_outstanding;
};

static int mbim_parse_descriptors(struct mbim_data *md, const char *file)
{
	void *data;
	size_t len;
	const struct mbim_desc *desc = NULL;
	const struct mbim_extended_desc *ext_desc = NULL;

	data = l_file_get_contents(file, &len);
	if (!data)
		return -EIO;

	if (!mbim_find_descriptors(data, len, &desc, &ext_desc)) {
		l_free(data);
		return -ENOENT;
	}

	if (desc)
		md->max_segment = L_LE16_TO_CPU(desc->wMaxControlMessage);

	if (ext_desc)
		md->max_outstanding = ext_desc->bMaxOutstandingCommandMessages;

	l_free(data);
	return 0;
}

static int mbim_probe(struct ofono_modem *modem)
{
	const char *descriptors;
	struct mbim_data *data;
	int err;

	DBG("%p", modem);

	descriptors = ofono_modem_get_string(modem, "DescriptorFile");

	if (!descriptors)
		return -EINVAL;

	data = l_new(struct mbim_data, 1);
	data->max_outstanding = 1;

	err = mbim_parse_descriptors(data, descriptors);
	if (err < 0) {
		DBG("Warning, unable to load descriptors, setting defaults");
		data->max_segment = 512;
	}

	DBG("MaxSegment: %d, MaxOutstanding: %d",
			data->max_segment, data->max_outstanding);

	ofono_modem_set_data(modem, data);

	return 0;
}

static void mbim_remove(struct ofono_modem *modem)
{
	struct mbim_data *data = ofono_modem_get_data(modem);

	DBG("%p", modem);

	ofono_modem_set_data(modem, NULL);
	l_free(data);
}

static int mbim_enable(struct ofono_modem *modem)
{
	const char *device;

	DBG("%p", modem);

	device = ofono_modem_get_string(modem, "Device");
	if (!device)
		return -EINVAL;

	DBG("%p", device);

	return -ENOTSUP;
}

static int mbim_disable(struct ofono_modem *modem)
{
	DBG("%p", modem);

	return -ENOTSUP;
}

static void mbim_set_online(struct ofono_modem *modem, ofono_bool_t online,
				ofono_modem_online_cb_t cb, void *user_data)
{
	struct cb_data *cbd = cb_data_new(cb, user_data);

	DBG("%p %s", modem, online ? "online" : "offline");

	CALLBACK_WITH_FAILURE(cb, cbd->data);
	l_free(cbd);
}

static void mbim_pre_sim(struct ofono_modem *modem)
{
	DBG("%p", modem);
}

static void mbim_post_sim(struct ofono_modem *modem)
{
	DBG("%p", modem);
}

static void mbim_post_online(struct ofono_modem *modem)
{
	DBG("%p", modem);
}

static struct ofono_modem_driver mbim_driver = {
	.name		= "mbim",
	.probe		= mbim_probe,
	.remove		= mbim_remove,
	.enable		= mbim_enable,
	.disable	= mbim_disable,
	.set_online	= mbim_set_online,
	.pre_sim	= mbim_pre_sim,
	.post_sim	= mbim_post_sim,
	.post_online	= mbim_post_online,
};

static int mbim_init(void)
{
	return ofono_modem_driver_register(&mbim_driver);
}

static void mbim_exit(void)
{
	ofono_modem_driver_unregister(&mbim_driver);
}

OFONO_PLUGIN_DEFINE(mbim, "MBIM modem driver", VERSION,
			OFONO_PLUGIN_PRIORITY_DEFAULT, mbim_init, mbim_exit)
