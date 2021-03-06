/*
 *
 * Copyright 2015, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
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
 *
 */

#include "src/core/channel/channel_stack.h"

#include <string.h>

#include <grpc/support/alloc.h>
#include <grpc/support/log.h>
#include "test/core/util/test_config.h"

#define LOG_TEST_NAME() gpr_log(GPR_INFO, "%s", __FUNCTION__)

static void channel_init_func(grpc_channel_element *elem,
                              const grpc_channel_args *args,
                              grpc_mdctx *metadata_context, int is_first,
                              int is_last) {
  GPR_ASSERT(args->num_args == 1);
  GPR_ASSERT(args->args[0].type == GRPC_ARG_INTEGER);
  GPR_ASSERT(0 == strcmp(args->args[0].key, "test_key"));
  GPR_ASSERT(args->args[0].value.integer == 42);
  GPR_ASSERT(is_first);
  GPR_ASSERT(is_last);
  *(int *)(elem->channel_data) = 0;
}

static void call_init_func(grpc_call_element *elem,
                           const void *server_transport_data,
                           grpc_transport_op *initial_op) {
  ++*(int *)(elem->channel_data);
  *(int *)(elem->call_data) = 0;
}

static void channel_destroy_func(grpc_channel_element *elem) {}

static void call_destroy_func(grpc_call_element *elem) {
  ++*(int *)(elem->channel_data);
}

static void call_func(grpc_call_element *elem, grpc_transport_op *op) {
  ++*(int *)(elem->call_data);
}

static void channel_func(grpc_channel_element *elem,
                         grpc_channel_element *from_elem, grpc_channel_op *op) {
  ++*(int *)(elem->channel_data);
}

static void test_create_channel_stack(void) {
  const grpc_channel_filter filter = {
      call_func, channel_func, sizeof(int), call_init_func, call_destroy_func,
      sizeof(int), channel_init_func, channel_destroy_func, "some_test_filter"};
  const grpc_channel_filter *filters = &filter;
  grpc_channel_stack *channel_stack;
  grpc_call_stack *call_stack;
  grpc_channel_element *channel_elem;
  grpc_call_element *call_elem;
  grpc_arg arg;
  grpc_channel_args chan_args;
  grpc_mdctx *metadata_context;
  int *channel_data;
  int *call_data;

  LOG_TEST_NAME();

  metadata_context = grpc_mdctx_create();

  arg.type = GRPC_ARG_INTEGER;
  arg.key = "test_key";
  arg.value.integer = 42;

  chan_args.num_args = 1;
  chan_args.args = &arg;

  channel_stack = gpr_malloc(grpc_channel_stack_size(&filters, 1));
  grpc_channel_stack_init(&filters, 1, &chan_args, metadata_context,
                          channel_stack);
  GPR_ASSERT(channel_stack->count == 1);
  channel_elem = grpc_channel_stack_element(channel_stack, 0);
  channel_data = (int *)channel_elem->channel_data;
  GPR_ASSERT(*channel_data == 0);

  call_stack = gpr_malloc(channel_stack->call_stack_size);
  grpc_call_stack_init(channel_stack, NULL, NULL, call_stack);
  GPR_ASSERT(call_stack->count == 1);
  call_elem = grpc_call_stack_element(call_stack, 0);
  GPR_ASSERT(call_elem->filter == channel_elem->filter);
  GPR_ASSERT(call_elem->channel_data == channel_elem->channel_data);
  call_data = (int *)call_elem->call_data;
  GPR_ASSERT(*call_data == 0);
  GPR_ASSERT(*channel_data == 1);

  grpc_call_stack_destroy(call_stack);
  gpr_free(call_stack);
  GPR_ASSERT(*channel_data == 2);

  grpc_channel_stack_destroy(channel_stack);
  gpr_free(channel_stack);

  grpc_mdctx_unref(metadata_context);
}

int main(int argc, char **argv) {
  grpc_test_init(argc, argv);
  test_create_channel_stack();
  return 0;
}
