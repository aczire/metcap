/**
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "kafka.h"
#include <Winsock2.h>

#define POLL_TIMEOUT_MS 1000

/*
* data structures required for the kafka client
*/
static rd_kafka_t** kaf_h;
static rd_kafka_topic_t** kaf_top_h;
static int num_conns;

/**
* A callback executed for each global Kafka option.
*/
static void kaf_global_option(const char* key, const char* val, void* arg)
{
	rd_kafka_conf_t* conf = (rd_kafka_conf_t*)arg;
	rd_kafka_conf_res_t rc;
	char err[512];

	rc = rd_kafka_conf_set(conf, key, val, err, sizeof(err));
	if (RD_KAFKA_CONF_OK != rc) {
		//LOG_WARN(USER1, "unable to set kafka global option: '%s' = '%s': %s\n", key, val, err);
		printf_s("unable to set kafka global option: '%s' = '%s': %s\n", key, val, err);
	}
}

/**
* A callback executed for topic-level Kafka option.
*/
static void kaf_topic_option(const char* key, const char* val, void* arg)
{
	rd_kafka_topic_conf_t* conf = (rd_kafka_topic_conf_t*)arg;
	rd_kafka_conf_res_t rc;
	char err[512];

	rc = rd_kafka_topic_conf_set(conf, key, val, err, sizeof(err));
	if (RD_KAFKA_CONF_OK != rc) {
		//LOG_WARN(USER1, "unable to set kafka topic option: '%s' = '%s': %s\n", key, val, err);
		printf_s("unable to set kafka topic option: '%s' = '%s': %s\n", key, val, err);
	}
}

static inline char* NextToken(char* pArg)
{
	// find next null with strchr and
	// point to the char beyond that.
	return strchr(pArg, '\0') + 1;
}

static void parse_kafka_config(char* file_path, const char* group,
	void(*option_cb)(const char* key, const char* val, void* arg), void* kaf_conf) {
	const int bufferSize = BUFF_SIZE;
	char buffer[BUFF_SIZE];

	int charsRead = 0;

	charsRead = GetPrivateProfileSectionA(group,
		buffer,
		bufferSize,
		file_path);

	if ((0 < charsRead) && ((bufferSize - 2) > charsRead)) {
		char* pSubstr = buffer;
		for (char* pToken = pSubstr; pToken && *pToken; pToken = NextToken(pToken)) {
			if ('\0' != *pToken && '#' != *pToken) {
				size_t substrLen = strlen(pToken);
				char* pos = strchr(pToken, '=');
				if (NULL != pos) {
					char key[MAX_LEN];
					char value[MAX_LEN];

					strncpy_s(key, _countof(key), pToken, pos - pToken);
					strncpy_s(value, _countof(value), pos + 1, substrLen - (pos - pToken));
					printf_s("config[%s]: %s = %s\n", group, key, value);
					option_cb(key, value, kaf_conf);
				}
			}
		}
	}
}

/**
* Initializes a pool of Kafka connections.
*/
int kaf_init(int num_of_conns, struct app_params app)
{
	int i;
	char errstr[512];

	// the number of connections to maintain
	num_conns = num_of_conns;

	// create kafka resources for each consumer
	kaf_h = calloc(num_of_conns, sizeof(rd_kafka_t*));
	kaf_top_h = calloc(num_of_conns, sizeof(rd_kafka_topic_t*));

	for (i = 0; i < num_of_conns; i++) {

		// configure kafka connection; values parsed from kafka config file
		rd_kafka_conf_t* kaf_conf = rd_kafka_conf_new();
		if (NULL != app.kafka_config_path) {
			parse_kafka_config(app.kafka_config_path, "kafka-global", kaf_global_option, (void*)kaf_conf);
		}

		// create a new kafka connection
		kaf_h[i] = rd_kafka_new(RD_KAFKA_PRODUCER, kaf_conf, errstr, sizeof(errstr));
		if (!kaf_h[i]) {
			//rte_exit(EXIT_FAILURE, "Cannot init kafka connection: %s", errstr);
			printf_s("Cannot init kafka connection: %s", errstr);
			return -1;
		}

		// configure kafka topic; values parsed from kafka config file
		rd_kafka_topic_conf_t* topic_conf = rd_kafka_topic_conf_new();
		if (NULL != app.kafka_config_path) {
			parse_kafka_config(app.kafka_config_path, "kafka-topic", kaf_topic_option, (void*)topic_conf);
		}

		// connect to a kafka topic
		kaf_top_h[i] = rd_kafka_topic_new(kaf_h[i], app.kafka_topic, topic_conf);
		if (!kaf_top_h[i]) {
			//rte_exit(EXIT_FAILURE, "Cannot init kafka topic: %s", app.kafka_topic);
			printf_s("Cannot init kafka topic: %s", app.kafka_topic);
			return -1;
		}
	}

	return 0;
}

/**
* Closes the pool of Kafka connections.
*/
void kaf_close(void)
{
	int i;
	for (i = 0; i < num_conns; i++) {
		// wait for messages to be delivered
		while (rd_kafka_outq_len(kaf_h[i]) > 0) {
			//LOG_INFO(USER1, "waiting for %d messages to clear on conn [%i/%i]",
			//rd_kafka_outq_len(kaf_h[i]), i + 1, num_conns);
			printf_s("waiting for %d messages to clear on conn [%i/%i]",
				rd_kafka_outq_len(kaf_h[i]), i + 1, num_conns);
			rd_kafka_poll(kaf_h[i], POLL_TIMEOUT_MS);
		}

		rd_kafka_topic_destroy(kaf_top_h[i]);
		rd_kafka_destroy(kaf_h[i]);
	}
}

/**
* The current time in microseconds.
*/
static uint64_t current_time(void)
{
	struct timeval tv;
	//gettimeofday(&tv, NULL);
	SYSTEMTIME  system_time;
	FILETIME    file_time;
	ULARGE_INTEGER ularge;

	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);
	ularge.LowPart = file_time.dwLowDateTime;
	ularge.HighPart = file_time.dwHighDateTime;

	tv.tv_sec = (long)((ularge.QuadPart - epoch) / 10000000L);
	tv.tv_usec = (long)(system_time.wMilliseconds * 1000);
	return tv.tv_sec * (uint64_t)1000000 + tv.tv_usec;
}

/**
* Publish a set of packets to a kafka topic.
*/
int kaf_send(unsigned char* data, unsigned long pkt_length, int conn_id)
{
	// unassigned partition
	int partition = RD_KAFKA_PARTITION_UA;
	int pkts_sent = 0;
	int drops;
	rd_kafka_message_t kaf_msg; // pkt_count

	// TODO: ensure that librdkafka cleans this up for us
	uint64_t *now = malloc(sizeof(uint64_t));

	// the current time in microseconds from the epoch (in big-endian aka network
	// byte order) is added as a message key before being sent to kafka
	// *now = htobe64(current_time());
	*now = _byteswap_uint64(current_time());

	// find the topic connection based on the conn_id
	rd_kafka_topic_t* kaf_topic = kaf_top_h[conn_id];

	// create the batch message for kafka
		kaf_msg.err = 0;
		kaf_msg.rkt = kaf_topic;
		kaf_msg.partition = partition;
		kaf_msg.payload = data;
		kaf_msg.len = pkt_length;
		kaf_msg.key = (void*)now;
		kaf_msg.key_len = sizeof(uint64_t);
		kaf_msg.offset = 0;

	// hand all of the messages off to kafka
	pkts_sent = rd_kafka_produce_batch(kaf_topic, partition, RD_KAFKA_MSG_F_COPY, &kaf_msg, 1);

	// did we drop packets?
	drops = pkts_sent - 1;
	if (drops > 0) {
			if (!kaf_msg.err) {
				//LOG_ERROR(USER1, "'%d' packets dropped, first error: %s", drops, (char*)kaf_msgs[i].payload);
				printf_s("'%d' packets dropped, first error: %s", drops, (char*)kaf_msg.payload);
			}
		}

	return pkts_sent;
}