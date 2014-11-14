// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstddef>
#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "testing/gtest/include/gtest/gtest.h"

#define I18N_ADDRESSINPUT_UTIL_BASICTYPES_H_
#include "third_party/libaddressinput/chromium/preload_address_validator.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/address_data.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/address_field.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/address_problem.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/address_validator.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/callback.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/downloader.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/null_storage.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/storage.h"
#include "third_party/libaddressinput/src/cpp/src/region_data_constants.h"
#include "third_party/libaddressinput/src/cpp/test/fake_downloader.h"

namespace {

using ::autofill::PreloadAddressValidator;
using ::i18n::addressinput::AddressData;
using ::i18n::addressinput::AddressField;
using ::i18n::addressinput::BuildCallback;
using ::i18n::addressinput::Downloader;
using ::i18n::addressinput::FakeDownloader;
using ::i18n::addressinput::FieldProblemMap;
using ::i18n::addressinput::NullStorage;
using ::i18n::addressinput::RegionDataConstants;
using ::i18n::addressinput::Storage;

using ::i18n::addressinput::COUNTRY;
using ::i18n::addressinput::ADMIN_AREA;
using ::i18n::addressinput::LOCALITY;
using ::i18n::addressinput::DEPENDENT_LOCALITY;
using ::i18n::addressinput::SORTING_CODE;
using ::i18n::addressinput::POSTAL_CODE;
using ::i18n::addressinput::STREET_ADDRESS;
using ::i18n::addressinput::RECIPIENT;

using ::i18n::addressinput::UNKNOWN_VALUE;
using ::i18n::addressinput::INVALID_FORMAT;
using ::i18n::addressinput::MISMATCHING_VALUE;

class PreloadAddressValidatorTest : public testing::Test {
 protected:
  PreloadAddressValidatorTest()
      : validator_(
          new PreloadAddressValidator(
              FakeDownloader::kFakeAggregateDataUrl,
              scoped_ptr<Downloader>(new FakeDownloader),
              scoped_ptr<Storage>(new NullStorage))),
        loaded_(BuildCallback(this, &PreloadAddressValidatorTest::Loaded)) {
    validator_->LoadRules("US", *loaded_);
  }

  virtual ~PreloadAddressValidatorTest() {}

  const scoped_ptr<PreloadAddressValidator> validator_;
  const scoped_ptr<PreloadAddressValidator::Callback> loaded_;

 private:
  void Loaded(bool success,
              const std::string& region_code,
              const int& rule_count) {
    AddressData address_data;
    address_data.region_code = region_code;
    FieldProblemMap dummy;
    PreloadAddressValidator::Status status =
        validator_->Validate(address_data, NULL, &dummy);
    ASSERT_EQ(success, status == PreloadAddressValidator::SUCCESS);
  }

  DISALLOW_COPY_AND_ASSIGN(PreloadAddressValidatorTest);
};

TEST_F(PreloadAddressValidatorTest, RegionHasRules) {
  const std::vector<std::string>& region_codes =
      RegionDataConstants::GetRegionCodes();
  AddressData address;
  for (size_t i = 0; i < region_codes.size(); ++i) {
    SCOPED_TRACE("For region: " + region_codes[i]);
    validator_->LoadRules(region_codes[i], *loaded_);
    address.region_code = region_codes[i];
    FieldProblemMap dummy;
    EXPECT_EQ(
        PreloadAddressValidator::SUCCESS,
        validator_->Validate(address, NULL, &dummy));
  }
}

TEST_F(PreloadAddressValidatorTest, EmptyAddressNoFatalFailure) {
  AddressData address;
  address.region_code = "US";

  FieldProblemMap dummy;
  EXPECT_EQ(
      PreloadAddressValidator::SUCCESS,
      validator_->Validate(address, NULL, &dummy));
}

TEST_F(PreloadAddressValidatorTest, USZipCode) {
  AddressData address;
  address.address_line.push_back("340 Main St.");
  address.locality = "Venice";
  address.administrative_area = "CA";
  address.region_code = "US";

  // Valid Californian zip code.
  address.postal_code = "90291";
  FieldProblemMap problems;
  EXPECT_EQ(
      PreloadAddressValidator::SUCCESS,
      validator_->Validate(address, NULL, &problems));
  EXPECT_TRUE(problems.empty());

  problems.clear();

  // An extended, valid Californian zip code.
  address.postal_code = "90210-1234";
  EXPECT_EQ(
      PreloadAddressValidator::SUCCESS,
      validator_->Validate(address, NULL, &problems));
  EXPECT_TRUE(problems.empty());

  problems.clear();

  // New York zip code (which is invalid for California).
  address.postal_code = "12345";
  EXPECT_EQ(
      PreloadAddressValidator::SUCCESS,
      validator_->Validate(address, NULL, &problems));
  EXPECT_EQ(1U, problems.size());
  EXPECT_EQ(problems.begin()->first, POSTAL_CODE);
  EXPECT_EQ(problems.begin()->second, MISMATCHING_VALUE);

  problems.clear();

  // A zip code with a "90" in the middle.
  address.postal_code = "12903";
  EXPECT_EQ(
      PreloadAddressValidator::SUCCESS,
      validator_->Validate(address, NULL, &problems));
  EXPECT_EQ(1U, problems.size());
  EXPECT_EQ(problems.begin()->first, POSTAL_CODE);
  EXPECT_EQ(problems.begin()->second, MISMATCHING_VALUE);

  problems.clear();

  // Invalid zip code (too many digits).
  address.postal_code = "902911";
  EXPECT_EQ(
      PreloadAddressValidator::SUCCESS,
      validator_->Validate(address, NULL, &problems));
  EXPECT_EQ(1U, problems.size());
  EXPECT_EQ(problems.begin()->first, POSTAL_CODE);
  EXPECT_EQ(problems.begin()->second, INVALID_FORMAT);

  problems.clear();

  // Invalid zip code (too few digits).
  address.postal_code = "9029";
  EXPECT_EQ(
      PreloadAddressValidator::SUCCESS,
      validator_->Validate(address, NULL, &problems));
  EXPECT_EQ(1U, problems.size());
  EXPECT_EQ(problems.begin()->first, POSTAL_CODE);
  EXPECT_EQ(problems.begin()->second, INVALID_FORMAT);
}

// Test case disabled because libaddressinput address validation doesn't do
// those kinds of normalizations that this test case expects. TODO: Something.
TEST_F(PreloadAddressValidatorTest, DISABLED_BasicValidation) {
  // US rules should always be available, even though this load call fails.
  validator_->LoadRules("US", *loaded_);
  AddressData address;
  address.region_code = "US";
  address.language_code = "en";
  address.administrative_area = "TX";
  address.locality = "Paris";
  address.postal_code = "75461";
  address.address_line.push_back("123 Main St");
  FieldProblemMap problems;
  EXPECT_EQ(
      PreloadAddressValidator::SUCCESS,
      validator_->Validate(address, NULL, &problems));
  EXPECT_TRUE(problems.empty());

  // The display name works as well as the key.
  address.administrative_area = "Texas";
  problems.clear();
  EXPECT_EQ(
      PreloadAddressValidator::SUCCESS,
      validator_->Validate(address, NULL, &problems));
  EXPECT_TRUE(problems.empty());

  // Ignore capitalization.
  address.administrative_area = "tx";
  problems.clear();
  EXPECT_EQ(
      PreloadAddressValidator::SUCCESS,
      validator_->Validate(address, NULL, &problems));
  EXPECT_TRUE(problems.empty());

  // Ignore capitalization.
  address.administrative_area = "teXas";
  problems.clear();
  EXPECT_EQ(
      PreloadAddressValidator::SUCCESS,
      validator_->Validate(address, NULL, &problems));
  EXPECT_TRUE(problems.empty());

  // Ignore diacriticals.
  address.administrative_area = "T\u00E9xas";
  problems.clear();
  EXPECT_EQ(
      PreloadAddressValidator::SUCCESS,
      validator_->Validate(address, NULL, &problems));
  EXPECT_TRUE(problems.empty());
}

TEST_F(PreloadAddressValidatorTest, BasicValidationFailure) {
  // US rules should always be available, even though this load call fails.
  validator_->LoadRules("US", *loaded_);
  AddressData address;
  address.region_code = "US";
  address.language_code = "en";
  address.administrative_area = "XT";
  address.locality = "Paris";
  address.postal_code = "75461";
  address.address_line.push_back("123 Main St");
  FieldProblemMap problems;
  EXPECT_EQ(
      PreloadAddressValidator::SUCCESS,
      validator_->Validate(address, NULL, &problems));

  ASSERT_EQ(1U, problems.size());
  EXPECT_EQ(UNKNOWN_VALUE, problems.begin()->second);
  EXPECT_EQ(ADMIN_AREA, problems.begin()->first);
}

TEST_F(PreloadAddressValidatorTest, NoNullSuggestionsCrash) {
  AddressData address;
  address.region_code = "US";
  EXPECT_EQ(PreloadAddressValidator::SUCCESS,
            validator_->GetSuggestions(address, COUNTRY, 1, NULL));
}

TEST_F(PreloadAddressValidatorTest, SuggestAdminAreaForPostalCode) {
  AddressData address;
  address.region_code = "US";
  address.postal_code = "90291";

  std::vector<AddressData> suggestions;
  EXPECT_EQ(PreloadAddressValidator::SUCCESS,
            validator_->GetSuggestions(address, POSTAL_CODE, 1, &suggestions));
  ASSERT_EQ(1U, suggestions.size());
  EXPECT_EQ("CA", suggestions[0].administrative_area);
  EXPECT_EQ("90291", suggestions[0].postal_code);
}

TEST_F(PreloadAddressValidatorTest, SuggestLocalityForPostalCodeWithAdminArea) {
  validator_->LoadRules("TW", *loaded_);
  AddressData address;
  address.region_code = "TW";
  address.postal_code = "515";
  address.administrative_area = "Changhua";

  std::vector<AddressData> suggestions;
  EXPECT_EQ(PreloadAddressValidator::SUCCESS,
            validator_->GetSuggestions(address, POSTAL_CODE, 1, &suggestions));
  ASSERT_EQ(1U, suggestions.size());
  EXPECT_EQ("Dacun Township", suggestions[0].locality);
  EXPECT_EQ("Changhua County", suggestions[0].administrative_area);
  EXPECT_EQ("515", suggestions[0].postal_code);
}

TEST_F(PreloadAddressValidatorTest, SuggestAdminAreaForPostalCodeWithLocality) {
  validator_->LoadRules("TW", *loaded_);
  AddressData address;
  address.region_code = "TW";
  address.postal_code = "515";
  address.locality = "Dacun";

  std::vector<AddressData> suggestions;
  EXPECT_EQ(PreloadAddressValidator::SUCCESS,
            validator_->GetSuggestions(address, POSTAL_CODE, 1, &suggestions));
  ASSERT_EQ(1U, suggestions.size());
  EXPECT_EQ("Dacun Township", suggestions[0].locality);
  EXPECT_EQ("Changhua County", suggestions[0].administrative_area);
  EXPECT_EQ("515", suggestions[0].postal_code);
}

TEST_F(PreloadAddressValidatorTest, NoSuggestForPostalCodeWithWrongAdminArea) {
  AddressData address;
  address.region_code = "US";
  address.postal_code = "90066";
  address.postal_code = "TX";

  std::vector<AddressData> suggestions;
  EXPECT_EQ(PreloadAddressValidator::SUCCESS,
            validator_->GetSuggestions(address, POSTAL_CODE, 1, &suggestions));
  EXPECT_TRUE(suggestions.empty());
}

TEST_F(PreloadAddressValidatorTest, SuggestForLocality) {
  validator_->LoadRules("CN", *loaded_);
  AddressData address;
  address.region_code = "CN";
  address.locality = "Anqin";

  std::vector<AddressData> suggestions;
  EXPECT_EQ(PreloadAddressValidator::SUCCESS,
            validator_->GetSuggestions(address, LOCALITY, 10, &suggestions));
  ASSERT_EQ(1U, suggestions.size());
  EXPECT_EQ("Anqing Shi", suggestions[0].locality);
  EXPECT_EQ("ANHUI SHENG", suggestions[0].administrative_area);
}

TEST_F(PreloadAddressValidatorTest, SuggestForLocalityAndAdminArea) {
  validator_->LoadRules("CN", *loaded_);
  AddressData address;
  address.region_code = "CN";
  address.locality = "Anqing";
  address.administrative_area = "Anhui";

  std::vector<AddressData> suggestions;
  EXPECT_EQ(PreloadAddressValidator::SUCCESS,
            validator_->GetSuggestions(address, LOCALITY, 10, &suggestions));
  ASSERT_EQ(1U, suggestions.size());
  EXPECT_TRUE(suggestions[0].dependent_locality.empty());
  EXPECT_EQ("Anqing Shi", suggestions[0].locality);
  EXPECT_EQ("ANHUI SHENG", suggestions[0].administrative_area);
}

TEST_F(PreloadAddressValidatorTest, SuggestForAdminAreaAndLocality) {
  validator_->LoadRules("CN", *loaded_);
  AddressData address;
  address.region_code = "CN";
  address.locality = "Anqing";
  address.administrative_area = "Anhui";

  std::vector<AddressData> suggestions;
  EXPECT_EQ(PreloadAddressValidator::SUCCESS,
            validator_->GetSuggestions(address, ADMIN_AREA, 10, &suggestions));
  ASSERT_EQ(1U, suggestions.size());
  EXPECT_TRUE(suggestions[0].dependent_locality.empty());
  EXPECT_TRUE(suggestions[0].locality.empty());
  EXPECT_EQ("ANHUI SHENG", suggestions[0].administrative_area);
}

TEST_F(PreloadAddressValidatorTest, SuggestForDependentLocality) {
  validator_->LoadRules("CN", *loaded_);
  AddressData address;
  address.region_code = "CN";
  address.dependent_locality = "Zongyang";

  std::vector<AddressData> suggestions;
  EXPECT_EQ(PreloadAddressValidator::SUCCESS,
            validator_->GetSuggestions(
                address, DEPENDENT_LOCALITY, 10, &suggestions));
  ASSERT_EQ(1U, suggestions.size());
  EXPECT_EQ("Zongyang Xian", suggestions[0].dependent_locality);
  EXPECT_EQ("Anqing Shi", suggestions[0].locality);
  EXPECT_EQ("ANHUI SHENG", suggestions[0].administrative_area);
}

TEST_F(PreloadAddressValidatorTest,
       NoSuggestForDependentLocalityWithWrongAdminArea) {
  validator_->LoadRules("CN", *loaded_);
  AddressData address;
  address.region_code = "CN";
  address.dependent_locality = "Zongyang";
  address.administrative_area = "Sichuan Sheng";

  std::vector<AddressData> suggestions;
  EXPECT_EQ(PreloadAddressValidator::SUCCESS,
            validator_->GetSuggestions(
                address, DEPENDENT_LOCALITY, 10, &suggestions));
  EXPECT_TRUE(suggestions.empty());
}

TEST_F(PreloadAddressValidatorTest, EmptySuggestionsOverLimit) {
  AddressData address;
  address.region_code = "US";
  address.administrative_area = "A";

  std::vector<AddressData> suggestions;
  EXPECT_EQ(PreloadAddressValidator::SUCCESS,
            validator_->GetSuggestions(address, ADMIN_AREA, 1, &suggestions));
  EXPECT_TRUE(suggestions.empty());
}

TEST_F(PreloadAddressValidatorTest, PreferShortSuggestions) {
  AddressData address;
  address.region_code = "US";
  address.administrative_area = "CA";

  std::vector<AddressData> suggestions;
  EXPECT_EQ(PreloadAddressValidator::SUCCESS,
            validator_->GetSuggestions(address, ADMIN_AREA, 10, &suggestions));
  ASSERT_EQ(1U, suggestions.size());
  EXPECT_EQ("CA", suggestions[0].administrative_area);
}

TEST_F(PreloadAddressValidatorTest, SuggestTheSingleMatchForFullMatchName) {
  AddressData address;
  address.region_code = "US";
  address.administrative_area = "Texas";

  std::vector<AddressData> suggestions;
  EXPECT_EQ(PreloadAddressValidator::SUCCESS,
            validator_->GetSuggestions(address, ADMIN_AREA, 10, &suggestions));
  ASSERT_EQ(1U, suggestions.size());
  EXPECT_EQ("Texas", suggestions[0].administrative_area);
}

TEST_F(PreloadAddressValidatorTest, SuggestAdminArea) {
  AddressData address;
  address.region_code = "US";
  address.administrative_area = "Cali";

  std::vector<AddressData> suggestions;
  EXPECT_EQ(PreloadAddressValidator::SUCCESS,
            validator_->GetSuggestions(address, ADMIN_AREA, 10, &suggestions));
  ASSERT_EQ(1U, suggestions.size());
  EXPECT_EQ("California", suggestions[0].administrative_area);
}

TEST_F(PreloadAddressValidatorTest, MultipleSuggestions) {
  AddressData address;
  address.region_code = "US";
  address.administrative_area = "MA";

  std::vector<AddressData> suggestions;
  EXPECT_EQ(PreloadAddressValidator::SUCCESS,
            validator_->GetSuggestions(address, ADMIN_AREA, 10, &suggestions));
  EXPECT_LT(1U, suggestions.size());

  // Massachusetts should not be a suggestion, because it's already covered
  // under MA.
  std::set<std::string> expected_suggestions;
  expected_suggestions.insert("MA");
  expected_suggestions.insert("Maine");
  expected_suggestions.insert("Marshall Islands");
  expected_suggestions.insert("Maryland");
  for (std::vector<AddressData>::const_iterator it = suggestions.begin();
       it != suggestions.end(); ++it) {
    expected_suggestions.erase(it->administrative_area);
  }
  EXPECT_TRUE(expected_suggestions.empty());
}

TEST_F(PreloadAddressValidatorTest, SuggestNonLatinKeyWhenLanguageMatches) {
  validator_->LoadRules("KR", *loaded_);
  AddressData address;
  address.language_code = "ko";
  address.region_code = "KR";
  address.postal_code = "210-210";

  std::vector<AddressData> suggestions;
  EXPECT_EQ(PreloadAddressValidator::SUCCESS,
            validator_->GetSuggestions(address, POSTAL_CODE, 1, &suggestions));
  ASSERT_EQ(1U, suggestions.size());
  EXPECT_EQ("강원도", suggestions[0].administrative_area);
  EXPECT_EQ("210-210", suggestions[0].postal_code);
}

TEST_F(PreloadAddressValidatorTest, SuggestNonLatinKeyWhenUserInputIsNotLatin) {
  validator_->LoadRules("KR", *loaded_);
  AddressData address;
  address.language_code = "en";
  address.region_code = "KR";
  address.administrative_area = "강원";

  std::vector<AddressData> suggestions;
  EXPECT_EQ(PreloadAddressValidator::SUCCESS,
            validator_->GetSuggestions(address, ADMIN_AREA, 1, &suggestions));
  ASSERT_EQ(1U, suggestions.size());
  EXPECT_EQ("강원도", suggestions[0].administrative_area);
}

TEST_F(PreloadAddressValidatorTest,
       SuggestLatinNameWhenLanguageDiffersAndLatinNameAvailable) {
  validator_->LoadRules("KR", *loaded_);
  AddressData address;
  address.language_code = "en";
  address.region_code = "KR";
  address.postal_code = "210-210";

  std::vector<AddressData> suggestions;
  EXPECT_EQ(PreloadAddressValidator::SUCCESS,
            validator_->GetSuggestions(address, POSTAL_CODE, 1, &suggestions));
  ASSERT_EQ(1U, suggestions.size());
  EXPECT_EQ("Gangwon", suggestions[0].administrative_area);
  EXPECT_EQ("210-210", suggestions[0].postal_code);
}

TEST_F(PreloadAddressValidatorTest, SuggestLatinNameWhenUserInputIsLatin) {
  validator_->LoadRules("KR", *loaded_);
  AddressData address;
  address.language_code = "ko";
  address.region_code = "KR";
  address.administrative_area = "Gang";

  std::vector<AddressData> suggestions;
  EXPECT_EQ(PreloadAddressValidator::SUCCESS,
            validator_->GetSuggestions(address, ADMIN_AREA, 1, &suggestions));
  ASSERT_EQ(1U, suggestions.size());
  EXPECT_EQ("Gangwon", suggestions[0].administrative_area);
}

TEST_F(PreloadAddressValidatorTest, NoSuggestionsForEmptyAddress) {
  AddressData address;
  address.region_code = "US";

  std::vector<AddressData> suggestions;
  EXPECT_EQ(
      PreloadAddressValidator::SUCCESS,
      validator_->GetSuggestions(address, POSTAL_CODE, 999, &suggestions));
  EXPECT_TRUE(suggestions.empty());
}

TEST_F(PreloadAddressValidatorTest, SuggestionIncludesCountry) {
  AddressData address;
  address.region_code = "US";
  address.postal_code = "90291";

  std::vector<AddressData> suggestions;
  EXPECT_EQ(PreloadAddressValidator::SUCCESS,
            validator_->GetSuggestions(address, POSTAL_CODE, 1, &suggestions));
  ASSERT_EQ(1U, suggestions.size());
  EXPECT_EQ("US", suggestions[0].region_code);
}

TEST_F(PreloadAddressValidatorTest,
       SuggestOnlyForAdministrativeAreasAndPostalCode) {
  AddressData address;
  address.region_code = "US";
  address.administrative_area = "CA";
  address.locality = "Los Angeles";
  address.dependent_locality = "Venice";
  address.postal_code = "90291";
  address.sorting_code = "123";
  address.address_line.push_back("123 Main St");
  address.recipient = "Jon Smith";

  // Fields that should not have suggestions in US.
  static const AddressField kNoSugestFields[] = {
    COUNTRY,
    LOCALITY,
    DEPENDENT_LOCALITY,
    SORTING_CODE,
    STREET_ADDRESS,
    RECIPIENT
  };

  static const size_t kNumNoSuggestFields =
      sizeof kNoSugestFields / sizeof (AddressField);

  for (size_t i = 0; i < kNumNoSuggestFields; ++i) {
    std::vector<AddressData> suggestions;
    EXPECT_EQ(PreloadAddressValidator::SUCCESS,
              validator_->GetSuggestions(
                  address, kNoSugestFields[i], 999, &suggestions));
    EXPECT_TRUE(suggestions.empty());
  }
}

TEST_F(PreloadAddressValidatorTest, CanonicalizeUsAdminAreaName) {
  AddressData address;
  address.region_code = "US";
  address.administrative_area = "cALIFORNIa";
  EXPECT_TRUE(validator_->CanonicalizeAdministrativeArea(&address));
  EXPECT_EQ("CA", address.administrative_area);
}

TEST_F(PreloadAddressValidatorTest, CanonicalizeUsAdminAreaKey) {
  AddressData address;
  address.region_code = "US";
  address.administrative_area = "CA";
  EXPECT_TRUE(validator_->CanonicalizeAdministrativeArea(&address));
  EXPECT_EQ("CA", address.administrative_area);
}

TEST_F(PreloadAddressValidatorTest, CanonicalizeJpAdminAreaKey) {
  validator_->LoadRules("JP", *loaded_);
  AddressData address;
  address.region_code = "JP";
  address.administrative_area = "東京都";
  EXPECT_TRUE(validator_->CanonicalizeAdministrativeArea(&address));
  EXPECT_EQ("東京都", address.administrative_area);
}

TEST_F(PreloadAddressValidatorTest, CanonicalizeJpAdminAreaLatinName) {
  validator_->LoadRules("JP", *loaded_);
  AddressData address;
  address.region_code = "JP";
  address.administrative_area = "tOKYo";
  EXPECT_TRUE(validator_->CanonicalizeAdministrativeArea(&address));
  EXPECT_EQ("TOKYO", address.administrative_area);
}

}  // namespace
