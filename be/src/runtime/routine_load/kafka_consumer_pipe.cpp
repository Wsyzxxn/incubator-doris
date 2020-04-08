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

#include "runtime/routine_load/kafka_consumer_pipe.h"

#include "gutil/strings/escaping.h"

namespace doris {

void KafkaConsumerPipe::read_field(std::map<std::string, std::string>& src,
                                   avro::GenericDatum& avro_union_data) {
    //get avro_map from avro_union of value
    //           {
    //              "type":"map",
    //              "values":[
    //                "string",
    //                "null"
    //              ]
    //            }
    avro::GenericDatum& avro_map_data =
            avro_union_data.selectUnionDatum(avro_union_data.unionBranch());

    for (auto& srckv : avro_map_data.value<avro::GenericMap>().value()) {
        //get avro_union from avro_map
        //          "values":[
        //             "string",
        //             "null"
        //          ]
        const avro::GenericDatum& avro_union_value =
                srckv.second.selectUnionDatum(srckv.second.unionBranch());

        // convert  union to string  and insert to src
        std::string key = srckv.first;
        if (avro_union_value.type() == avro::AVRO_STRING) {
            src[key] = avro_union_value.value<std::string>();
        } else {
            // deal null
            src[key] = "";
        }
    }
}

//now avro json schema is hard code
Status KafkaConsumerPipe::append_avro_bytes(const uint8_t* data, size_t size) {
    avro::GenericDatum datum(_avro_jdwdata_schema);
    // avro binaryDocoder
    std::unique_ptr<avro::InputStream> json_in = avro::memoryInputStream(data, size);
    avro::DecoderPtr d = avro::binaryDecoder();
    d->init(*json_in);
    avro::decode(*d, datum);

    if (datum.type() == avro::AVRO_RECORD) {
        avro::GenericRecord& genericdecord = datum.value<avro::GenericRecord>();
        if (config::avro_bytes_src_field_index >= genericdecord.fieldCount() ||
            config::avro_bytes_cur_field_index >= genericdecord.fieldCount()) {
            return Status::InternalError(
                    "your set avro bytes field index is big than record field count!");
        }
        std::map<std::string, std::string> src;
        //get union for recode data   (config::jdwdatafile)
        //        {
        //          "name":"cur",
        //          "type":[
        //            {
        //              "type":"map",
        //              "values":[
        //                "string",
        //                "null"
        //              ]
        //            },
        //            "null"
        //          ]
        //        }
        read_field(src, genericdecord.fieldAt(config::avro_bytes_src_field_index));
        read_field(src, genericdecord.fieldAt(config::avro_bytes_cur_field_index));
        return append_map(src);
    }
    return Status::InternalError("not import type of " + datum.type());
}

Status KafkaConsumerPipe::append_avro_json(const char* data, size_t size) {
    std::map<std::string, std::string> src;
    rapidjson::Document document;
    if (!document.Parse(data, size).HasParseError()) {
        if (document.HasMember("src")) {
            set_map(src, document["src"].MemberBegin(), document["src"].MemberEnd());
        } else {
            return Status::InternalError("Parse avro json data hasn't src member!");
        }
        if (document.HasMember("cur")) {
            set_map(src, document["cur"].MemberBegin(), document["cur"].MemberEnd());
        }
        return append_map(src);
    }
    return Status::InternalError("Parse avro json data failed!");
}

void KafkaConsumerPipe::set_map(std::map<std::string, std::string>& src,
                                rapidjson::Value::ConstMemberIterator itr,
                                rapidjson::Value::ConstMemberIterator member_end) {
    for (; itr != member_end; ++itr) {
        if (itr->value.IsString()) {
            src[itr->name.GetString()] = itr->value.GetString();
        } else if (itr->value.IsTrue()) {
            src[itr->name.GetString()] = "true";
        } else if (itr->value.IsFalse()) {
            src[itr->name.GetString()] = "false";
        } else if (itr->value.IsInt()) {
            src[itr->name.GetString()] = std::to_string(itr->value.GetInt());
        } else if (itr->value.IsInt64()) {
            src[itr->name.GetString()] = std::to_string(itr->value.GetInt64());
        } else if (itr->value.IsDouble()) {
            src[itr->name.GetString()] = std::to_string(itr->value.GetDouble());
        } else if (itr->value.IsNull()) {
            src[itr->name.GetString()] = "NULL";
        } else {
            src[itr->name.GetString()] = "";
        }
    }
}

Status KafkaConsumerPipe::append_map(const std::map<std::string, std::string>& src) {
    // sdata : stringbuilder
    if (src.empty()) {
        return Status::OK();
    }
    std::stringstream sdata("");
    auto src_current = src.begin();
    sdata << "{";
    for (int i = src.size(); i > 1; --i, ++src_current) {
        sdata << "\"" << strings::Utf8SafeCEscape(src_current->first) << "\":"
              << "\"" << strings::Utf8SafeCEscape(src_current->second) << "\",";
    }
    sdata << "\"" << strings::Utf8SafeCEscape(src_current->first) << "\":"
          << "\"" << strings::Utf8SafeCEscape(src_current->second) << "\"}";
    return append_data(sdata.str().c_str(), sdata.str().length());
}

Status KafkaConsumerPipe::append_data(const char* line, size_t size) {
    Status st = append(line, size);
    if (!st.ok()) {
        return st;
    }
    return append("\n", 1);
}

Status KafkaConsumerPipe::append_with_line_delimiter(void* data, size_t size,
                                                     std::string data_type) {
    if (data_type == "avro_bytes") {
        return append_avro_bytes(static_cast<const uint8_t*>(data), size);
    } else if (data_type == "avro_json") {
        return append_avro_json(static_cast<const char*>(data), size);
    }
    return append_data(static_cast<const char*>(data), size);
}
} // namespace doris