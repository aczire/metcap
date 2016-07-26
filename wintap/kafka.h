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

#ifndef METRON_KAFKA_H
#define METRON_KAFKA_H

#include <string.h>
#include <stdlib.h> 
#include <malloc.h>
#include <rdkafka.h>

 /* FILETIME of Jan 1 1970 00:00:00. */
static const unsigned __int64 epoch = ((unsigned __int64)116444736000000000ULL);

/**
* Application configuration parameters.
*/
struct app_params {
	uint32_t enabled_port_mask;
	char* kafka_topic;
	char* kafka_config_path;
} __rte_cache_aligned;

struct wtap_mbuf {
	uint32_t enabled_port_mask;
	char* kafka_topic;
	char* kafka_config_path;
} __wtap_mbuf;

/*
* Contains all application parameters.
*/
struct app_params app;

/**
 * Initializes a pool of Kafka connections.
 */
void kaf_init(int num_of_conns);

/**
 * Publish a set of packets to a kafka topic.
 */
int kaf_send(struct wtap_mbuf* data, int num_to_send, int conn_id);

/**
 * Closes the pool of Kafka connections.
 */
void kaf_close(void);

#endif
