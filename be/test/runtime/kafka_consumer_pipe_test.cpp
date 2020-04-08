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

#include <gtest/gtest.h>

namespace doris {

class KafkaConsumerPipeTest : public testing::Test {
public:
    KafkaConsumerPipeTest() { }
    virtual ~ KafkaConsumerPipeTest() { }

    void SetUp() override {


    }

    void TearDown() override {

    }

private:

};

TEST_F(KafkaConsumerPipeTest, append_read) {
    KafkaConsumerPipe k_pipe(1024 * 1024, 64 * 1024);

    std::string msg1 = "i have a dream";
    std::string msg2 = "This is from kafka";
    std::string msg3 = R"json(
{
    "mid":"mm",
    "db":"kawmsidc1_report",
    "sch":"kawmsidc1_report",
    "tab":"ob_locate_d",
    "opt":"UPDATE",
    "ts":"mmm",
    "ddl":"mmm",
    "err":"mmm",
    "src":{
        "title":"i have a dream "
    },
    "cur":{
        "title":"i have a dream  this if from kakfka"
    },
    "cus":{
        "modified":"0",
        "box_qty":"songenjie"
    }
})json"
    std::string msg4 = R"json({"a","i have a dream"})json"
    std::string msg5 ="i have \"\n dream";
    msg5 = k_pipe.string_escape(msg5, true);

    std::string msg6 = "i have \"\n dream";
    msg6 = k_pipe.string_escape(msg5, false);

    Status st;
    st = k_pipe.append_with_line_delimiter(msg1.c_str(), msg1.length());
    ASSERT_TRUE(st.ok());
    st = k_pipe.append_with_line_delimiter(msg2.c_str(), msg2.length());
    ASSERT_TRUE(st.ok());
    st = k_pipe.append_with_line_delimiter(msg3.c_str(), msg3.length(), "avro_json");
    ASSERT_TRUE(st.ok());
    st = k_pipe.append_with_line_delimiter(msg4.c_str(), msg4.length(), "json");
    ASSERT_TRUE(st.ok());
    st = k_pipe.finish();
    ASSERT_TRUE(st.ok());

    char buf[1024];
    size_t data_size = 1024;
    bool eof = false;
    st = k_pipe.read((uint8_t*)buf, &data_size, &eof);
    ASSERT_TRUE(st.ok());
    ASSERT_EQ(data_size, msg1.length() + msg2.length() + 2);
    ASSERT_EQ(eof, false);

    data_size = 1024;
    st = k_pipe.read((uint8_t*)buf, &data_size, &eof);
    ASSERT_TRUE(st.ok());
    ASSERT_EQ(data_size, 0);
    ASSERT_EQ(eof, true);

    data_size = 1024;
    st = k_pipe.read((uint8_t*)buf, &data_size, &eof);
    ASSERT_TRUE(st.ok());
    ASSERT_EQ(data_size, 0);
    ASSERT_EQ(eof, true);

    data_size = 1024;
    st = k_pipe.read((uint8_t*)buf, &data_size, &eof);
    ASSERT_TRUE(st.ok());
    ASSERT_EQ(data_size, 0);
    ASSERT_EQ(eof, true);

    if (msg5 == "i have \\\"\\n dream") {
        ASSERT_TRUE(st.ok());
    } else {
        ASSERT_TRUE(Status::InternalError("string escape error!" + msg5));
    }

    if (msg6 == "i have \"\\n dream") {
        ASSERT_TRUE(st.ok());
    } else {
        ASSERT_TRUE(Status::InternalError("string escape error!" + msg6));
    }

}

}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    doris::CpuInfo::init();
    return RUN_ALL_TESTS();
}

