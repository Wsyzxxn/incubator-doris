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

package org.apache.doris.metric;

/*
 * Gauge metric is updated every time it is visited
 */
public class GaugeMetric<T> extends Metric<T> {
    /**
     * Construct an instance with specified name and description
     *
     * @param name
     * @param description
     * @return
     */
    public GaugeMetric(String name, MetricUnit unit, String description) {
        super(name, MetricType.GAUGE, unit, description);
    }

    private T value;

    public void setValue(T v) {
        this.value = v;
    }

    @Override
    public T getValue() {
        return value;
    }
}

