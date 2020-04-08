// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#pragma once

#include <stdint.h>

#include <cinttypes>
#include <map>
#include <string>
#include <vector>
//these two have to be ifdef guarded becuase JS support compiler versions where
//they are not implemented
#include <fstream>

#include "exec/file_reader.h"
#include "librdkafka/rdkafka.h"
#include "runtime/message_body_sink.h"
#include "runtime/stream_load/stream_load_pipe.h"

// avro | json decode
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stream.h>
#include <rapidjson/stringbuffer.h>

#include "avro/Compiler.hh"
#include "avro/Decoder.hh"
#include "avro/Encoder.hh"
#include "avro/Generic.hh"
#include "avro/Specific.hh"
#include "json_struct.h"
#include "runtime/stream_load/stream_load_context.h"
namespace doris {
class KafkaConsumerPipe : public StreamLoadPipe {
public:
    KafkaConsumerPipe(size_t max_buffered_bytes = 1024 * 1024, size_t min_chunk_size = 64 * 1024)
            : StreamLoadPipe(max_buffered_bytes, min_chunk_size) {
        // Get Json Schema
        std::ifstream ifs(config::jdwdatafile);
        /*{
            "name":"",
            "namespace":"",
            "fields":[
            {
                "name":"mid",
                "type":"long"
            },
            {
                "name":"db",
                "type":"string"
            },
            {
                "name":"sch",
                "type":"string"
            },
            {
                "name":"tab",
                "type":"string"
            },
            {
                "name":"opt",
                "type":"string"
            },
            {
                "name":"ts",
                "type":"long"
            },
            {
                "name":"ddl",
                "type":[
                "string",
                "null"
                ]
            },
            {
                "name":"err",
                "type":[
                "string",
                "null"
                ]
            },
            {
                "name":"src",     #we need number 8
                "type":[
                {
                    "type":"map",
                    "values":[
                    "string",
                    "null"
                    ]
                },
                "null"
                ]
            },
            {
                "name":"cur",      #we need number 9
                "type":[
                {
                    "type":"map",
                    "values":[
                    "string",
                    "null"
                    ]
                },
                "null"
                ]
            },
            {
                "name":"cus",
                "type":[
                {
                    "type":"map",
                    "values":[
                    "string",
                    "null"
                    ]
                },
                "null"
                ]
            }
            ]
        }
        */
        avro::compileJsonSchema(ifs, _avro_jdwdata_schema);
    }

    virtual ~KafkaConsumerPipe() {}

    Status append_with_line_delimiter(void* data, size_t size, std::string data_type);

private:
    //avro bytes
    avro::ValidSchema _avro_jdwdata_schema;

    Status append_avro_bytes(const uint8_t* data, size_t size);

    Status append_avro_json(const char* data, size_t size);

    Status append_map(const std::map<std::string, std::string>& src);

    Status append_data(const char* line, size_t size);

    void read_field(std::map<std::string, std::string>& src, avro::GenericDatum& avro_union_data);

    std::string string_escape(std::string& str, bool if_need_escape_quotation_mark);

    void set_map(std::map<std::string, std::string>& src, rapidjson::Value::ConstMemberIterator itr,
                 rapidjson::Value::ConstMemberIterator member_end);
};
} // end namespace doris
