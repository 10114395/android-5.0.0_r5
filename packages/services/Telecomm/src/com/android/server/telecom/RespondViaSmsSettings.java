/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.server.telecom;

import android.app.ActionBar;
import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.EditTextPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.view.Menu;
import android.view.MenuItem;

// TODO: Needed for move to system service: import com.android.internal.R;

/**
 * Helper class to manage the "Respond via SMS Message" feature for incoming calls.
 */
public class RespondViaSmsSettings {
    private static final String KEY_PREFERRED_PACKAGE = "preferred_package_pref";
    private static final String KEY_INSTANT_TEXT_DEFAULT_COMPONENT = "instant_text_def_component";

    // TODO: This class is newly copied into Telecom (com.android.server.telecom) from it previous
    // location in Telephony (com.android.phone). User's preferences stored in the old location
    // will be lost. We need code here to migrate KLP -> LMP settings values.

    /**
     * Settings activity under "Call settings" to let you manage the
     * canned responses; see respond_via_sms_settings.xml
     */
    public static class Settings extends PreferenceActivity
            implements Preference.OnPreferenceChangeListener {
        @Override
        protected void onCreate(Bundle icicle) {
            super.onCreate(icicle);
            Log.d(this, "Settings: onCreate()...");

            // This function guarantees that QuickResponses will be in our
            // SharedPreferences with the proper values considering there may be
            // old QuickResponses in Telephony pre L.
            QuickResponseUtils.maybeMigrateLegacyQuickResponses(this);

            getPreferenceManager().setSharedPreferencesName(
                    QuickResponseUtils.SHARED_PREFERENCES_NAME);

            // This preference screen is ultra-simple; it's just 4 plain
            // <EditTextPreference>s, one for each of the 4 "canned responses".
            //
            // The only nontrivial thing we do here is copy the text value of
            // each of those EditTextPreferences and use it as the preference's
            // "title" as well, so that the user will immediately see all 4
            // strings when they arrive here.
            //
            // Also, listen for change events (since we'll need to update the
            // title any time the user edits one of the strings.)

            addPreferencesFromResource(R.xml.respond_via_sms_settings);

            EditTextPreference pref;
            pref = (EditTextPreference) findPreference(
                    QuickResponseUtils.KEY_CANNED_RESPONSE_PREF_1);
            pref.setTitle(pref.getText());
            pref.setOnPreferenceChangeListener(this);

            pref = (EditTextPreference) findPreference(
                    QuickResponseUtils.KEY_CANNED_RESPONSE_PREF_2);
            pref.setTitle(pref.getText());
            pref.setOnPreferenceChangeListener(this);

            pref = (EditTextPreference) findPreference(
                    QuickResponseUtils.KEY_CANNED_RESPONSE_PREF_3);
            pref.setTitle(pref.getText());
            pref.setOnPreferenceChangeListener(this);

            pref = (EditTextPreference) findPreference(
                    QuickResponseUtils.KEY_CANNED_RESPONSE_PREF_4);
            pref.setTitle(pref.getText());
            pref.setOnPreferenceChangeListener(this);

            ActionBar actionBar = getActionBar();
            if (actionBar != null) {
                // android.R.id.home will be triggered in onOptionsItemSelected()
                actionBar.setDisplayHomeAsUpEnabled(true);
            }
        }

        // Preference.OnPreferenceChangeListener implementation
        @Override
        public boolean onPreferenceChange(Preference preference, Object newValue) {
            Log.d(this, "onPreferenceChange: key = %s", preference.getKey());
            Log.d(this, "  preference = '%s'", preference);
            Log.d(this, "  newValue = '%s'", newValue);

            EditTextPreference pref = (EditTextPreference) preference;

            // Copy the new text over to the title, just like in onCreate().
            // (Watch out: onPreferenceChange() is called *before* the
            // Preference itself gets updated, so we need to use newValue here
            // rather than pref.getText().)
            pref.setTitle((String) newValue);

            return true;  // means it's OK to update the state of the Preference with the new value
        }

        @Override
        public boolean onOptionsItemSelected(MenuItem item) {
            final int itemId = item.getItemId();
            switch (itemId) {
                case android.R.id.home:
                    goUpToTopLevelSetting(this);
                    return true;
                case R.id.respond_via_message_reset:
                    // Reset the preferences settings
                    SharedPreferences prefs = getSharedPreferences(
                            QuickResponseUtils.SHARED_PREFERENCES_NAME, Context.MODE_PRIVATE);
                    SharedPreferences.Editor editor = prefs.edit();
                    editor.remove(KEY_INSTANT_TEXT_DEFAULT_COMPONENT);
                    editor.apply();

                    return true;
                default:
            }
            return super.onOptionsItemSelected(item);
        }

        @Override
        public boolean onCreateOptionsMenu(Menu menu) {
            getMenuInflater().inflate(R.menu.respond_via_message_settings_menu, menu);
            return super.onCreateOptionsMenu(menu);
        }
    }

    /**
     * Finish current Activity and go up to the top level Settings.
     */
    public static void goUpToTopLevelSetting(Activity activity) {
        activity.finish();
     }
}
