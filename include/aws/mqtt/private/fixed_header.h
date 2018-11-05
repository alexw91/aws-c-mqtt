#ifndef AWS_MQTT_PRIVATE_FIXED_HEADER_H
#define AWS_MQTT_PRIVATE_FIXED_HEADER_H

/*
 * Copyright 2010-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <aws/common/byte_buf.h>

#include <aws/mqtt/mqtt.h>

/* Represents the types of the MQTT control packets [MQTT-2.2.1]. */
enum aws_mqtt_packet_type {
    /* reserved = 0, */
    AWS_MQTT_PACKET_CONNECT = 1,
    AWS_MQTT_PACKET_CONNACK,
    AWS_MQTT_PACKET_PUBLISH,
    AWS_MQTT_PACKET_PUBACK,
    AWS_MQTT_PACKET_PUBREC,
    AWS_MQTT_PACKET_PUBREL,
    AWS_MQTT_PACKET_PUBCOMP,
    AWS_MQTT_PACKET_SUBSCRIBE,
    AWS_MQTT_PACKET_SUBACK,
    AWS_MQTT_PACKET_UNSUBSCRIBE,
    AWS_MQTT_PACKET_UNSUBACK,
    AWS_MQTT_PACKET_PINGREQ,
    AWS_MQTT_PACKET_PINGRESP,
    AWS_MQTT_PACKET_DISCONNECT,
    /* reserved = 15, */
};

/**
 * Represents the fixed header [MQTT-2.2].
 */
struct aws_mqtt_fixed_header {
    enum aws_mqtt_packet_type packet_type;
    size_t remaining_length;
    uint8_t flags;
};

/**
 * Get the type of packet from the first byte of the buffer [MQTT-2.2.1].
 */
enum aws_mqtt_packet_type aws_mqtt_get_packet_type(const uint8_t *buffer);

/**
 * Get traits describing a packet described by header [MQTT-2.2.2].
 */
bool aws_mqtt_packet_has_flags(const struct aws_mqtt_fixed_header *header);

/**
 * Write a fixed header to a byte stream.
 */
int aws_mqtt_fixed_header_encode(struct aws_byte_cursor *cur, const struct aws_mqtt_fixed_header *header);

/**
 * Read a fixed header from a byte stream.
 */
int aws_mqtt_fixed_header_decode(struct aws_byte_cursor *cur, struct aws_mqtt_fixed_header *header);

#endif /* AWS_MQTT_PRIVATE_FIXED_HEADER_H */