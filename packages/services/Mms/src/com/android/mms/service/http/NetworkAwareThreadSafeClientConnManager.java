/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.mms.service.http;

import org.apache.http.conn.ClientConnectionOperator;
import org.apache.http.conn.scheme.SchemeRegistry;
import org.apache.http.impl.conn.tsccm.ThreadSafeClientConnManager;
import org.apache.http.params.HttpParams;

/**
 * This is a subclass of {@link org.apache.http.impl.conn.tsccm.ThreadSafeClientConnManager}
 * which allows us to specify a custom name resolver and the address type
 */
public class NetworkAwareThreadSafeClientConnManager extends ThreadSafeClientConnManager {
    public NetworkAwareThreadSafeClientConnManager(HttpParams params,
            SchemeRegistry schreg, NameResolver resolver, boolean shouldUseIpv6) {
        super(params, schreg);
        ((NetworkAwareClientConnectionOperator)connOperator).setNameResolver(resolver);
        ((NetworkAwareClientConnectionOperator)connOperator).setShouldUseIpv6(shouldUseIpv6);
    }

    @Override
    protected ClientConnectionOperator createConnectionOperator(SchemeRegistry schreg) {
        return new NetworkAwareClientConnectionOperator(schreg);
    }
}
