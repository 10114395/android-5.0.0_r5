// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/browser/data_reduction_proxy_settings.h"

#include "base/command_line.h"
#include "base/md5.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "components/data_reduction_proxy/browser/data_reduction_proxy_params.h"
#include "components/data_reduction_proxy/browser/data_reduction_proxy_settings_test_utils.h"
#include "components/data_reduction_proxy/common/data_reduction_proxy_pref_names.h"
#include "components/data_reduction_proxy/common/data_reduction_proxy_switches.h"
#include "net/http/http_auth.h"
#include "net/http/http_auth_cache.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

const char kDataReductionProxy[] = "https://foo.com:443/";
const char kDataReductionProxyDev[] = "http://foo-dev.com:80";
const char kDataReductionProxyFallback[] = "http://bar.com:80";
const char kDataReductionProxyKey[] = "12345";
const char kDataReductionProxyAlt[] = "https://alt.com:443/";
const char kDataReductionProxyAltFallback[] = "http://alt2.com:80";
const char kDataReductionProxySSL[] = "http://ssl.com:80";

const char kProbeURLWithOKResponse[] = "http://ok.org/";
const char kProbeURLWithBadResponse[] = "http://bad.org/";
const char kProbeURLWithNoResponse[] = "http://no.org/";
const char kWarmupURLWithNoContentResponse[] = "http://warm.org/";

}  // namespace

namespace data_reduction_proxy {

class DataReductionProxySettingsTest
    : public ConcreteDataReductionProxySettingsTest<
          DataReductionProxySettings> {
};


TEST_F(DataReductionProxySettingsTest, TestAuthenticationInit) {
  net::HttpAuthCache cache;
  DataReductionProxyParams drp_params(
      DataReductionProxyParams::kAllowed |
      DataReductionProxyParams::kFallbackAllowed |
      DataReductionProxyParams::kPromoAllowed);
  drp_params.set_key(kDataReductionProxyKey);
  DataReductionProxySettings::InitDataReductionAuthentication(
      &cache, &drp_params);
  DataReductionProxyParams::DataReductionProxyList proxies =
      drp_params.GetAllowedProxies();
  for (DataReductionProxyParams::DataReductionProxyList::iterator it =
           proxies.begin();  it != proxies.end(); ++it) {
    net::HttpAuthCache::Entry* entry = cache.LookupByPath(*it,
                                                          std::string("/"));
    EXPECT_TRUE(entry != NULL);
    EXPECT_EQ(net::HttpAuth::AUTH_SCHEME_SPDYPROXY, entry->scheme());
    EXPECT_EQ("SpdyProxy", entry->auth_challenge().substr(0,9));
  }
  GURL bad_server = GURL("https://bad.proxy.com/");
  net::HttpAuthCache::Entry* entry =
      cache.LookupByPath(bad_server, std::string());
  EXPECT_TRUE(entry == NULL);
}

TEST_F(DataReductionProxySettingsTest, TestGetDataReductionProxyOrigin) {
  // SetUp() adds the origin to the command line, which should be returned here.
  std::string result =
      settings_->params()->origin().spec();
  EXPECT_EQ(GURL(kDataReductionProxy), GURL(result));
}

TEST_F(DataReductionProxySettingsTest, TestGetDataReductionProxyDevOrigin) {
  CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      switches::kDataReductionProxyDev, kDataReductionProxyDev);
  ResetSettings(true, true, false, true);
  std::string result =
      settings_->params()->origin().spec();
  EXPECT_EQ(GURL(kDataReductionProxyDev), GURL(result));
}


TEST_F(DataReductionProxySettingsTest, TestGetDataReductionProxies) {
  DataReductionProxyParams drp_params(
      DataReductionProxyParams::kAllowed |
      DataReductionProxyParams::kFallbackAllowed |
      DataReductionProxyParams::kPromoAllowed);
  DataReductionProxyParams::DataReductionProxyList proxies =
      drp_params.GetAllowedProxies();

  unsigned int expected_proxy_size = 2u;
  EXPECT_EQ(expected_proxy_size, proxies.size());

  // Command line proxies have precedence, so even if there were other values
  // compiled in, these should be the ones in the list.
  EXPECT_EQ("foo.com", proxies[0].host());
  EXPECT_EQ(443 ,proxies[0].EffectiveIntPort());
  EXPECT_EQ("bar.com", proxies[1].host());
  EXPECT_EQ(80, proxies[1].EffectiveIntPort());
}

TEST_F(DataReductionProxySettingsTest, TestAuthHashGeneration) {
  std::string salt = "8675309";  // Jenny's number to test the hash generator.
  std::string salted_key = salt + kDataReductionProxyKey + salt;
  base::string16 expected_hash = base::UTF8ToUTF16(base::MD5String(salted_key));
  EXPECT_EQ(expected_hash,
            DataReductionProxySettings::AuthHashForSalt(
                8675309, kDataReductionProxyKey));
}

TEST_F(DataReductionProxySettingsTest, TestSetProxyConfigs) {
  CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      switches::kDataReductionProxyAlt, kDataReductionProxyAlt);
  CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      switches::kDataReductionProxyAltFallback, kDataReductionProxyAltFallback);
  CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      switches::kDataReductionSSLProxy, kDataReductionProxySSL);
  ResetSettings(true, true, true, true);
  TestDataReductionProxyConfig* config =
      static_cast<TestDataReductionProxyConfig*>(
          settings_->configurator());

  settings_->SetProxyConfigs(true, true, false, false);
  EXPECT_TRUE(config->enabled_);
  EXPECT_TRUE(net::HostPortPair::FromString(kDataReductionProxyAlt).Equals(
                  net::HostPortPair::FromString(config->origin_)));
  EXPECT_TRUE(
      net::HostPortPair::FromString(kDataReductionProxyAltFallback).Equals(
          net::HostPortPair::FromString(config->fallback_origin_)));
  EXPECT_TRUE(net::HostPortPair::FromString(kDataReductionProxySSL).Equals(
                  net::HostPortPair::FromString(config->ssl_origin_)));

  settings_->SetProxyConfigs(true, false, false, false);
  EXPECT_TRUE(config->enabled_);
  EXPECT_TRUE(net::HostPortPair::FromString(kDataReductionProxy).Equals(
                  net::HostPortPair::FromString(config->origin_)));
  EXPECT_TRUE(net::HostPortPair::FromString(kDataReductionProxyFallback).Equals(
                  net::HostPortPair::FromString(config->fallback_origin_)));
  EXPECT_EQ("", config->ssl_origin_);

  settings_->SetProxyConfigs(false, true, false, false);
  EXPECT_FALSE(config->enabled_);
  EXPECT_EQ("", config->origin_);
  EXPECT_EQ("", config->fallback_origin_);
  EXPECT_EQ("", config->ssl_origin_);

  settings_->SetProxyConfigs(false, false, false, false);
  EXPECT_FALSE(config->enabled_);
  EXPECT_EQ("", config->origin_);
  EXPECT_EQ("", config->fallback_origin_);
  EXPECT_EQ("", config->ssl_origin_);
}

TEST_F(DataReductionProxySettingsTest, TestIsProxyEnabledOrManaged) {
  settings_->InitPrefMembers();
  base::MessageLoopForUI loop;
  // The proxy is disabled initially.
  settings_->enabled_by_user_ = false;
  settings_->SetProxyConfigs(false, false, false, false);

  EXPECT_FALSE(settings_->IsDataReductionProxyEnabled());
  EXPECT_FALSE(settings_->IsDataReductionProxyManaged());

  CheckOnPrefChange(true, true, false);
  EXPECT_TRUE(settings_->IsDataReductionProxyEnabled());
  EXPECT_FALSE(settings_->IsDataReductionProxyManaged());

  CheckOnPrefChange(true, true, true);
  EXPECT_TRUE(settings_->IsDataReductionProxyEnabled());
  EXPECT_TRUE(settings_->IsDataReductionProxyManaged());

  base::MessageLoop::current()->RunUntilIdle();
}

TEST_F(DataReductionProxySettingsTest, TestAcceptableChallenges) {
  typedef struct {
    std::string host;
    std::string realm;
    bool expected_to_succeed;
  } challenge_test;

  challenge_test tests[] = {
    {"foo.com:443", "", false},                 // 0. No realm.
    {"foo.com:443", "xxx", false},              // 1. Wrong realm.
    {"foo.com:443", "spdyproxy", false},        // 2. Case matters.
    {"foo.com:443", "SpdyProxy", true},         // 3. OK.
    {"foo.com:443", "SpdyProxy1234567", true},  // 4. OK
    {"bar.com:80", "SpdyProxy1234567", true},   // 5. OK.
    {"foo.com:443", "SpdyProxyxxx", true},      // 6. OK
    {"", "SpdyProxy1234567", false},            // 7. No challenger.
    {"xxx.net:443", "SpdyProxy1234567", false}, // 8. Wrong host.
    {"foo.com", "SpdyProxy1234567", false},     // 9. No port.
    {"foo.com:80", "SpdyProxy1234567", false},  // 10.Wrong port.
    {"bar.com:81", "SpdyProxy1234567", false},  // 11.Wrong port.
  };

  for (int i = 0; i <= 11; ++i) {
    scoped_refptr<net::AuthChallengeInfo> auth_info(new net::AuthChallengeInfo);
    auth_info->challenger = net::HostPortPair::FromString(tests[i].host);
    auth_info->realm = tests[i].realm;
    EXPECT_EQ(tests[i].expected_to_succeed,
              settings_->IsAcceptableAuthChallenge(auth_info.get()));
  }
}

TEST_F(DataReductionProxySettingsTest, TestChallengeTokens) {
  typedef struct {
    std::string realm;
    bool expected_empty_token;
  } token_test;

  token_test tests[] = {
    {"", true},                  // 0. No realm.
    {"xxx", true},               // 1. realm too short.
    {"spdyproxy", true},         // 2. no salt.
    {"SpdyProxyxxx", true},      // 3. Salt not an int.
    {"SpdyProxy1234567", false}, // 4. OK
  };

  for (int i = 0; i <= 4; ++i) {
    scoped_refptr<net::AuthChallengeInfo> auth_info(new net::AuthChallengeInfo);
    auth_info->challenger =
        net::HostPortPair::FromString(kDataReductionProxy);
    auth_info->realm = tests[i].realm;
    base::string16 token = settings_->GetTokenForAuthChallenge(auth_info.get());
    EXPECT_EQ(tests[i].expected_empty_token, token.empty());
  }
}

TEST_F(DataReductionProxySettingsTest, TestResetDataReductionStatistics) {
  int64 original_content_length;
  int64 received_content_length;
  int64 last_update_time;
  settings_->ResetDataReductionStatistics();
  settings_->GetContentLengths(kNumDaysInHistory,
                               &original_content_length,
                               &received_content_length,
                               &last_update_time);
  EXPECT_EQ(0L, original_content_length);
  EXPECT_EQ(0L, received_content_length);
  EXPECT_EQ(last_update_time_.ToInternalValue(), last_update_time);
}

TEST_F(DataReductionProxySettingsTest, TestContentLengths) {
  int64 original_content_length;
  int64 received_content_length;
  int64 last_update_time;

  // Request |kNumDaysInHistory| days.
  settings_->GetContentLengths(kNumDaysInHistory,
                               &original_content_length,
                               &received_content_length,
                               &last_update_time);
  const unsigned int days = kNumDaysInHistory;
  // Received content length history values are 0 to |kNumDaysInHistory - 1|.
  int64 expected_total_received_content_length = (days - 1L) * days / 2;
  // Original content length history values are 0 to
  // |2 * (kNumDaysInHistory - 1)|.
  long expected_total_original_content_length = (days - 1L) * days;
  EXPECT_EQ(expected_total_original_content_length, original_content_length);
  EXPECT_EQ(expected_total_received_content_length, received_content_length);
  EXPECT_EQ(last_update_time_.ToInternalValue(), last_update_time);

  // Request |kNumDaysInHistory - 1| days.
  settings_->GetContentLengths(kNumDaysInHistory - 1,
                               &original_content_length,
                               &received_content_length,
                               &last_update_time);
  expected_total_received_content_length -= (days - 1);
  expected_total_original_content_length -= 2 * (days - 1);
  EXPECT_EQ(expected_total_original_content_length, original_content_length);
  EXPECT_EQ(expected_total_received_content_length, received_content_length);

  // Request 0 days.
  settings_->GetContentLengths(0,
                               &original_content_length,
                               &received_content_length,
                               &last_update_time);
  expected_total_received_content_length = 0;
  expected_total_original_content_length = 0;
  EXPECT_EQ(expected_total_original_content_length, original_content_length);
  EXPECT_EQ(expected_total_received_content_length, received_content_length);

  // Request 1 day. First day had 0 bytes so should be same as 0 days.
  settings_->GetContentLengths(1,
                               &original_content_length,
                               &received_content_length,
                               &last_update_time);
  EXPECT_EQ(expected_total_original_content_length, original_content_length);
  EXPECT_EQ(expected_total_received_content_length, received_content_length);
}

// TODO(marq): Add a test to verify that MaybeActivateDataReductionProxy
// is called when the pref in |settings_| is enabled.
TEST_F(DataReductionProxySettingsTest, TestMaybeActivateDataReductionProxy) {
  // Initialize the pref member in |settings_| without the usual callback
  // so it won't trigger MaybeActivateDataReductionProxy when the pref value
  // is set.
  settings_->spdy_proxy_auth_enabled_.Init(
      prefs::kDataReductionProxyEnabled,
      settings_->GetOriginalProfilePrefs());
  settings_->data_reduction_proxy_alternative_enabled_.Init(
      prefs::kDataReductionProxyAltEnabled,
      settings_->GetOriginalProfilePrefs());

  // TODO(bengr): Test enabling/disabling while a probe is outstanding.
  base::MessageLoopForUI loop;
  // The proxy is enabled and unrestructed initially.
  // Request succeeded but with bad response, expect proxy to be restricted.
  CheckProbe(true,
             kProbeURLWithBadResponse,
             kWarmupURLWithNoContentResponse,
             "Bad",
             true,
             true,
             true,
             false);
  // Request succeeded with valid response, expect proxy to be unrestricted.
  CheckProbe(true,
             kProbeURLWithOKResponse,
             kWarmupURLWithNoContentResponse,
             "OK",
             true,
             true,
             false,
             false);
  // Request failed, expect proxy to be enabled but restricted.
  CheckProbe(true,
             kProbeURLWithNoResponse,
             kWarmupURLWithNoContentResponse,
             "",
             false,
             true,
             true,
             false);
  // The proxy is disabled initially. Probes should not be emitted to change
  // state.
  CheckProbe(false,
             kProbeURLWithOKResponse,
             kWarmupURLWithNoContentResponse,
             "OK",
             true,
             false,
             false,
             false);
}

TEST_F(DataReductionProxySettingsTest, TestOnIPAddressChanged) {
  base::MessageLoopForUI loop;
  // The proxy is enabled initially.
  pref_service_.SetBoolean(prefs::kDataReductionProxyEnabled, true);
  settings_->spdy_proxy_auth_enabled_.Init(
      prefs::kDataReductionProxyEnabled,
      settings_->GetOriginalProfilePrefs());
  settings_->data_reduction_proxy_alternative_enabled_.Init(
      prefs::kDataReductionProxyAltEnabled,
      settings_->GetOriginalProfilePrefs());
  settings_->enabled_by_user_ = true;
  settings_->restricted_by_carrier_ = false;
  settings_->SetProxyConfigs(true, false, false, true);
  // IP address change triggers a probe that succeeds. Proxy remains
  // unrestricted.
  CheckProbeOnIPChange(kProbeURLWithOKResponse,
                       kWarmupURLWithNoContentResponse,
                       "OK",
                       true,
                       false,
                       false);
  // IP address change triggers a probe that fails. Proxy is restricted.
  CheckProbeOnIPChange(kProbeURLWithBadResponse,
                       kWarmupURLWithNoContentResponse,
                       "Bad",
                       true,
                       true,
                       false);
  // IP address change triggers a probe that fails. Proxy remains restricted.
  CheckProbeOnIPChange(kProbeURLWithBadResponse,
                       kWarmupURLWithNoContentResponse,
                       "Bad",
                       true,
                       true,
                       false);
  // IP address change triggers a probe that succeed. Proxy is unrestricted.
  CheckProbeOnIPChange(kProbeURLWithBadResponse,
                       kWarmupURLWithNoContentResponse,
                       "OK",
                       true,
                       false,
                       false);
}

TEST_F(DataReductionProxySettingsTest, TestOnProxyEnabledPrefChange) {
  settings_->InitPrefMembers();
  base::MessageLoopForUI loop;
  // The proxy is enabled initially.
  settings_->enabled_by_user_ = true;
  settings_->SetProxyConfigs(true, false, false, true);
  // The pref is disabled, so correspondingly should be the proxy.
  CheckOnPrefChange(false, false, false);
  // The pref is enabled, so correspondingly should be the proxy.
  CheckOnPrefChange(true, true, false);
}

TEST_F(DataReductionProxySettingsTest, TestInitDataReductionProxyOn) {
  MockSettings* settings = static_cast<MockSettings*>(settings_.get());
  EXPECT_CALL(*settings, RecordStartupState(PROXY_ENABLED));

  pref_service_.SetBoolean(prefs::kDataReductionProxyEnabled, true);
  CheckInitDataReductionProxy(true);
}

TEST_F(DataReductionProxySettingsTest, TestInitDataReductionProxyOff) {
  // InitDataReductionProxySettings with the preference off will directly call
  // LogProxyState.
  MockSettings* settings = static_cast<MockSettings*>(settings_.get());
  EXPECT_CALL(*settings, RecordStartupState(PROXY_DISABLED));

  pref_service_.SetBoolean(prefs::kDataReductionProxyEnabled, false);
  CheckInitDataReductionProxy(false);
}

TEST_F(DataReductionProxySettingsTest, TestSetProxyFromCommandLine) {
  MockSettings* settings = static_cast<MockSettings*>(settings_.get());
  EXPECT_CALL(*settings, RecordStartupState(PROXY_ENABLED));

  CommandLine::ForCurrentProcess()->AppendSwitch(
      switches::kEnableDataReductionProxy);
  CheckInitDataReductionProxy(true);
}

TEST_F(DataReductionProxySettingsTest, TestGetDailyContentLengths) {
  DataReductionProxySettings::ContentLengthList result =
      settings_->GetDailyContentLengths(prefs::kDailyHttpOriginalContentLength);

  ASSERT_FALSE(result.empty());
  ASSERT_EQ(kNumDaysInHistory, result.size());

  for (size_t i = 0; i < kNumDaysInHistory; ++i) {
    long expected_length =
        static_cast<long>((kNumDaysInHistory - 1 - i) * 2);
    ASSERT_EQ(expected_length, result[i]);
  }
}

TEST_F(DataReductionProxySettingsTest, CheckInitMetricsWhenNotAllowed) {
  // No call to |AddProxyToCommandLine()| was made, so the proxy feature
  // should be unavailable.
  base::MessageLoopForUI loop;
  // Clear the command line. Setting flags can force the proxy to be allowed.
  CommandLine::ForCurrentProcess()->InitFromArgv(0, NULL);

  ResetSettings(false, false, false, false);
  MockSettings* settings = static_cast<MockSettings*>(settings_.get());
  EXPECT_FALSE(settings->params()->allowed());
  EXPECT_CALL(*settings, RecordStartupState(PROXY_NOT_AVAILABLE));

  scoped_ptr<DataReductionProxyConfigurator> configurator(
      new TestDataReductionProxyConfig());
  settings_->SetProxyConfigurator(configurator.Pass());
  scoped_refptr<net::TestURLRequestContextGetter> request_context =
      new net::TestURLRequestContextGetter(base::MessageLoopProxy::current());
  settings_->InitDataReductionProxySettings(&pref_service_,
                                            &pref_service_,
                                            request_context.get());

  base::MessageLoop::current()->RunUntilIdle();
}

}  // namespace data_reduction_proxy
