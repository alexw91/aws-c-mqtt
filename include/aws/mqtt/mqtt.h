#ifndef AWS_MQTT_MQTT_H
#define AWS_MQTT_MQTT_H

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

#include <aws/mqtt/exports.h>

#include <aws/common/byte_buf.h>
#include <aws/common/string.h>

#include <aws/io/event_loop.h>
#include <aws/io/host_resolver.h>

/* forward declares */
struct aws_client_bootstrap;
struct aws_socket_endpoint;
struct aws_socket_options;
struct aws_tls_ctx_options;

/* Quality of Service associated with a publish action or subscription [MQTT-4.3]. */
enum aws_mqtt_qos {
    AWS_MQTT_QOS_AT_MOST_ONCE = 0,
    AWS_MQTT_QOS_AT_LEAST_ONCE = 1,
    AWS_MQTT_QOS_EXACTLY_ONCE = 2,
    /* reserved = 3 */
};

/* Result of a connect request [MQTT-3.2.2.3]. */
enum aws_mqtt_connect_return_code {
    AWS_MQTT_CONNECT_ACCEPTED,
    AWS_MQTT_CONNECT_UNACCEPTABLE_PROTOCOL_VERSION,
    AWS_MQTT_CONNECT_IDENTIFIER_REJECTED,
    AWS_MQTT_CONNECT_SERVER_UNAVAILABLE,
    AWS_MQTT_CONNECT_BAD_USERNAME_OR_PASSWORD,
    AWS_MQTT_CONNECT_NOT_AUTHORIZED,
    /* reserved = 6 - 255 */
};

struct aws_mqtt_client {
    struct aws_allocator *allocator;
    struct aws_event_loop_group *event_loop_group;
    struct aws_hash_table hosts_to_bootstrap;

    /* DNS Resolver */
    struct aws_host_resolver host_resolver;
    struct aws_host_resolution_config host_resolver_config;
};

struct aws_mqtt_client_connection;

/** Callback called when a request roundtrip is complete (QoS0 immediately, QoS1 on PUBACK, QoS2 on PUBCOMP). */
typedef void(aws_mqtt_op_complete_fn)(struct aws_mqtt_client_connection *connection, void *userdata);

/** Type of function called when a publish recieved matches a subscription */
typedef void(aws_mqtt_publish_recieved_fn)(
    struct aws_mqtt_client_connection *connection,
    const struct aws_byte_cursor *topic,
    const struct aws_byte_cursor *payload,
    void *user_data);

struct aws_mqtt_client_connection_callbacks {
    /* Called if the connection to the server is not completed.
     * Note that if a CONNACK is recieved, this function will not be called no matter what the return code is */
    void (*on_connection_failed)(struct aws_mqtt_client_connection *connection, int error_code, void *user_data);
    /* Called when a connection acknowlegement is received.
     * If return_code is not ACCEPT, the connetion is automatically closed. */
    void (*on_connack)(
        struct aws_mqtt_client_connection *connection,
        enum aws_mqtt_connect_return_code return_code,
        bool session_present,
        void *user_data);
    /* Called when a connection is closed, right before any resources are deleted. */
    void (*on_disconnect)(struct aws_mqtt_client_connection *connection, int error_code, void *user_data);

    void *user_data;
};

enum aws_mqtt_error {
    AWS_ERROR_MQTT_INVALID_RESERVED_BITS = 0x1400,
    AWS_ERROR_MQTT_BUFFER_TOO_BIG,
    AWS_ERROR_MQTT_INVALID_REMAINING_LENGTH,
    AWS_ERROR_MQTT_UNSUPPORTED_PROTOCOL_NAME,
    AWS_ERROR_MQTT_UNSUPPORTED_PROTOCOL_LEVEL,
    AWS_ERROR_MQTT_INVALID_CREDENTIALS,
    AWS_ERROR_MQTT_INVALID_QOS,
    AWS_ERROR_MQTT_INVALID_PACKET_TYPE,
    AWS_ERROR_MQTT_TIMEOUT,
    AWS_ERROR_MQTT_PROTOCOL_ERROR,

    AWS_ERROR_END_MQTT_RANGE = 0x1800,
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes an instance of aws_mqtt_client.
 * It is expected that the user will manage the client's object lifetime, this function will not allocate one.
 *
 * \param[in] client    The client to initialize
 * \param[in] allocator The allocator the client will use for all future allocations
 * \param[in] elg       The event loop group to distribute new connections on
 *
 * \returns AWS_OP_SUCCESS if successfully initialized, otherwise AWS_OP_ERR and aws_last_error() will be set.
 */
AWS_MQTT_API
int aws_mqtt_client_init(
    struct aws_mqtt_client *client,
    struct aws_allocator *allocator,
    struct aws_event_loop_group *elg);

/**
 * Cleans up and frees all memory allocated by the client.
 *
 * Note that calling this function before all connections are closed is undefined behavior.
 *
 * \param[in] client    The client to shut down
 */
AWS_MQTT_API
void aws_mqtt_client_clean_up(struct aws_mqtt_client *client);

/**
 * Spawns a new connection object.
 *
 * \param[in] client            The client to spawn the connection from
 * \param[in] callbacks         \see aws_mqtt_client_connection_callbacks
 * \param[in] host_name         The server name to connect to
 * \param[in] port              The port on the server to connect to
 * \param[in] socket_options    The socket options to pass to the aws_client_bootstrap functions
 * \param[in] tls_options       TLS settings to use when opening a connection.
 *                                  Pass NULL to connect without TLS (NOT RECOMMENDED)
 *
 * \returns AWS_OP_SUCCESS on success, otherwise AWS_OP_ERR and aws_last_error() is set.
 */
AWS_MQTT_API
struct aws_mqtt_client_connection *aws_mqtt_client_connection_new(
    struct aws_mqtt_client *client,
    struct aws_mqtt_client_connection_callbacks callbacks,
    const struct aws_byte_cursor *host_name,
    uint16_t port,
    struct aws_socket_options *socket_options,
    struct aws_tls_ctx_options *tls_options);

/**
 * Opens the actual connection defined by aws_mqtt_client_connection_new.
 * Once the connection is opened, on_connack will be called.
 *
 * \param[in] connection        The connection object
 * \param[in] client_id         The clientid to place in the CONNECT packet
 * \param[in] clean_session     True to discard all server session data and start fresh
 * \param[in] keep_alive_time   The keep alive value to place in the CONNECT PACKET
 *
 * \returns AWS_OP_SUCCESS if the connection has been successfully initiated,
 *              otherwise AWS_OP_ERR and aws_last_error() will be set.
 */
AWS_MQTT_API
int aws_mqtt_client_connection_connect(
    struct aws_mqtt_client_connection *connection,
    const struct aws_byte_cursor *client_id,
    bool clean_session,
    uint16_t keep_alive_time);

/**
 * Closes the connection asyncronously, calls the on_disconnect callback, and destroys the connection object.
 *
 * \param[in] connection    The connection to close
 *
 * \returns AWS_OP_SUCCESS if the connection is open and is being shutdown,
 *              otherwise AWS_OP_ERR and aws_last_error() is set.
 */
AWS_MQTT_API
int aws_mqtt_client_connection_disconnect(struct aws_mqtt_client_connection *connection);

/**
 * Subscrube to a topic filter. on_publish will be called when a PUBLISH matching topic_filter is recieved.
 *
 * \param[in] connection    The connection to subscribe on
 * \param[in] topic_filter  The topic filter to subscribe on
 * \param[in] qos           The maximum QoS of messages to recieve
 * \param[in] on_publish    Called when a PUBLISH packet matching topic_filter is recieved
 * \param[in] on_publish_ud Passed to on_publish
 * \param[in] on_suback     Called when a SUBACK has been recieved from the server and the subscription is complete
 * \param[in] on_suback_ud  Passed to on_suback
 *
 * \returns AWS_OP_SUCCESS if the connection is open and the SUBSCRIBE is sent or queued to send,
 *              otherwise AWS_OP_ERR and aws_last_error() is set.
 */
AWS_MQTT_API
int aws_mqtt_client_connection_subscribe(
    struct aws_mqtt_client_connection *connection,
    const struct aws_byte_cursor *topic_filter,
    enum aws_mqtt_qos qos,
    aws_mqtt_publish_recieved_fn *on_publish,
    void *on_publish_ud,
    aws_mqtt_op_complete_fn *on_suback,
    void *on_suback_ud);

/**
 * Unsubscribe to a topic filter.
 *
 * \param[in] connection        The connection to unsubscribe on
 * \param[in] topic_filter      The topic filter to unsubscribe on
 * \param[in] on_unsuback       Called when a UNSUBACK has been recieved from the server and the subscription is removed
 * \param[in] on_unsuback_ud    Passed to on_unsuback
 *
 * \returns AWS_OP_SUCCESS if the connection is open and the UNSUBSCRIBE is sent or queued to send,
 *              otherwise AWS_OP_ERR and aws_last_error() is set.
 */
AWS_MQTT_API
int aws_mqtt_client_connection_unsubscribe(
    struct aws_mqtt_client_connection *connection,
    const struct aws_byte_cursor *topic_filter,
    aws_mqtt_op_complete_fn *on_unsuback,
    void *on_unsuback_ud);

/**
 * Send a PUBLSIH packet over connection.
 *
 * \param[in] connection    The connection to publish on
 * \param[in] topic         The topic to publish on
 * \param[in] qos           The requested QoS of the packet
 * \param[in] retain        True to have the server save the packet, and send to all new subscriptions matching topic
 * \param[in] payload       The data to send as the payload of the publish
 * \param[in] on_complete   For QoS 0, called as soon as the packet is sent
 *                          For QoS 1, called when PUBACK is recieved
 *                          For QoS 2, called when PUBCOMP is recieved
 *
 * \returns AWS_OP_SUCCESS if the connection is open and the PUBLISH is sent or queued to send,
 *              otherwise AWS_OP_ERR and aws_last_error() is set.
 */
AWS_MQTT_API
int aws_mqtt_client_connection_publish(
    struct aws_mqtt_client_connection *connection,
    const struct aws_byte_cursor *topic,
    enum aws_mqtt_qos qos,
    bool retain,
    const struct aws_byte_cursor *payload,
    aws_mqtt_op_complete_fn *on_complete,
    void *userdata);

/**
 * Sends a PINGREQ packet to the server to keep the connection alive.
 * If a PINGRESP is not recieved within a reasonable period of time, the connection will be closed.
 *
 * \params[in] connection   The connection to ping on
 *
 * \returns AWS_OP_SUCCESS if the connection is open and the PINGREQ is sent or queued to send,
 *              otherwise AWS_OP_ERR and aws_last_error() is set.
 */
AWS_MQTT_API
int aws_mqtt_client_connection_ping(struct aws_mqtt_client_connection *connection);

/*
 * Loads error strings for debugging and logging purposes.
 */
AWS_MQTT_API
void aws_mqtt_load_error_strings(void);

#ifdef __cplusplus
}
#endif

#endif /* AWS_MQTT_MQTT_H */
