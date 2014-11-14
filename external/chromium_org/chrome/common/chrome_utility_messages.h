// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Multiply-included message file, so no include guard.

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/files/file_path.h"
#include "base/platform_file.h"
#include "base/strings/string16.h"
#include "base/tuple.h"
#include "base/values.h"
#include "chrome/common/extensions/update_manifest.h"
#include "chrome/common/media_galleries/iphoto_library.h"
#include "chrome/common/media_galleries/itunes_library.h"
#include "chrome/common/media_galleries/metadata_types.h"
#include "chrome/common/media_galleries/picasa_types.h"
#include "chrome/common/safe_browsing/zip_analyzer.h"
#include "ipc/ipc_message_macros.h"
#include "ipc/ipc_platform_file.h"
#include "third_party/skia/include/core/SkBitmap.h"

#define IPC_MESSAGE_START ChromeUtilityMsgStart

#ifndef CHROME_COMMON_CHROME_UTILITY_MESSAGES_H_
#define CHROME_COMMON_CHROME_UTILITY_MESSAGES_H_

typedef std::vector<Tuple2<SkBitmap, base::FilePath> > DecodedImages;

#endif  // CHROME_COMMON_CHROME_UTILITY_MESSAGES_H_

IPC_STRUCT_TRAITS_BEGIN(UpdateManifest::Result)
  IPC_STRUCT_TRAITS_MEMBER(extension_id)
  IPC_STRUCT_TRAITS_MEMBER(version)
  IPC_STRUCT_TRAITS_MEMBER(browser_min_version)
  IPC_STRUCT_TRAITS_MEMBER(package_hash)
  IPC_STRUCT_TRAITS_MEMBER(crx_url)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(UpdateManifest::Results)
  IPC_STRUCT_TRAITS_MEMBER(list)
  IPC_STRUCT_TRAITS_MEMBER(daystart_elapsed_seconds)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(safe_browsing::zip_analyzer::Results)
  IPC_STRUCT_TRAITS_MEMBER(success)
  IPC_STRUCT_TRAITS_MEMBER(has_executable)
  IPC_STRUCT_TRAITS_MEMBER(has_archive)
IPC_STRUCT_TRAITS_END()

#if defined(OS_MACOSX)
IPC_STRUCT_TRAITS_BEGIN(iphoto::parser::Photo)
  IPC_STRUCT_TRAITS_MEMBER(id)
  IPC_STRUCT_TRAITS_MEMBER(location)
  IPC_STRUCT_TRAITS_MEMBER(original_location)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(iphoto::parser::Library)
  IPC_STRUCT_TRAITS_MEMBER(albums)
  IPC_STRUCT_TRAITS_MEMBER(all_photos)
IPC_STRUCT_TRAITS_END()
#endif  // defined(OS_MACOSX)

#if defined(OS_WIN) || defined(OS_MACOSX)
IPC_STRUCT_TRAITS_BEGIN(itunes::parser::Track)
  IPC_STRUCT_TRAITS_MEMBER(id)
  IPC_STRUCT_TRAITS_MEMBER(location)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(picasa::AlbumInfo)
  IPC_STRUCT_TRAITS_MEMBER(name)
  IPC_STRUCT_TRAITS_MEMBER(timestamp)
  IPC_STRUCT_TRAITS_MEMBER(uid)
  IPC_STRUCT_TRAITS_MEMBER(path)
IPC_STRUCT_TRAITS_END()

// These files are opened read-only. Please see the constructor for
// picasa::AlbumTableFiles for details.
IPC_STRUCT_TRAITS_BEGIN(picasa::AlbumTableFilesForTransit)
  IPC_STRUCT_TRAITS_MEMBER(indicator_file)
  IPC_STRUCT_TRAITS_MEMBER(category_file)
  IPC_STRUCT_TRAITS_MEMBER(date_file)
  IPC_STRUCT_TRAITS_MEMBER(filename_file)
  IPC_STRUCT_TRAITS_MEMBER(name_file)
  IPC_STRUCT_TRAITS_MEMBER(token_file)
  IPC_STRUCT_TRAITS_MEMBER(uid_file)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(picasa::FolderINIContents)
  IPC_STRUCT_TRAITS_MEMBER(folder_path)
  IPC_STRUCT_TRAITS_MEMBER(ini_contents)
IPC_STRUCT_TRAITS_END()
#endif  // defined(OS_WIN) || defined(OS_MACOSX)

#if !defined(OS_ANDROID) && !defined(OS_IOS)
IPC_STRUCT_TRAITS_BEGIN(metadata::AttachedImage)
  IPC_STRUCT_TRAITS_MEMBER(type)
  IPC_STRUCT_TRAITS_MEMBER(data)
IPC_STRUCT_TRAITS_END()
#endif  // !defined(OS_ANDROID) && !defined(OS_IOS)

//------------------------------------------------------------------------------
// Utility process messages:
// These are messages from the browser to the utility process.

// Tells the utility process to unpack the given extension file in its
// directory and verify that it is valid.
IPC_MESSAGE_CONTROL4(ChromeUtilityMsg_UnpackExtension,
                     base::FilePath /* extension_filename */,
                     std::string /* extension_id */,
                     int /* Manifest::Location */,
                     int /* InitFromValue flags */)

// Tell the utility process to parse the given JSON data and verify its
// validity.
IPC_MESSAGE_CONTROL1(ChromeUtilityMsg_UnpackWebResource,
                     std::string /* JSON data */)

// Tell the utility process to parse the given xml document.
IPC_MESSAGE_CONTROL1(ChromeUtilityMsg_ParseUpdateManifest,
                     std::string /* xml document contents */)

// Tell the utility process to decode the given image data.
IPC_MESSAGE_CONTROL1(ChromeUtilityMsg_DecodeImage,
                     std::vector<unsigned char>)  // encoded image contents

// Tell the utility process to decode the given image data, which is base64
// encoded.
IPC_MESSAGE_CONTROL1(ChromeUtilityMsg_DecodeImageBase64,
                     std::string)  // base64 encoded image contents

// Tell the utility process to decode the given JPEG image data with a robust
// libjpeg codec.
IPC_MESSAGE_CONTROL1(ChromeUtilityMsg_RobustJPEGDecodeImage,
                     std::vector<unsigned char>)  // encoded image contents

// Tell the utility process to parse a JSON string into a Value object.
IPC_MESSAGE_CONTROL1(ChromeUtilityMsg_ParseJSON,
                     std::string /* JSON to parse */)

// Tell the utility process to patch the given |input_file| using |patch_file|
// and place the output in |output_file|. The patch should use the bsdiff
// algorithm (Courgette's version).
IPC_MESSAGE_CONTROL3(ChromeUtilityMsg_PatchFileBsdiff,
                     base::FilePath /* input_file */,
                     base::FilePath /* patch_file */,
                     base::FilePath /* output_file */)

// Tell the utility process to patch the given |input_file| using |patch_file|
// and place the output in |output_file|. The patch should use the Courgette
// algorithm.
IPC_MESSAGE_CONTROL3(ChromeUtilityMsg_PatchFileCourgette,
                     base::FilePath /* input_file */,
                     base::FilePath /* patch_file */,
                     base::FilePath /* output_file */)

#if defined(OS_CHROMEOS)
// Tell the utility process to create a zip file on the given list of files.
IPC_MESSAGE_CONTROL3(ChromeUtilityMsg_CreateZipFile,
                     base::FilePath /* src_dir */,
                     std::vector<base::FilePath> /* src_relative_paths */,
                     base::FileDescriptor /* dest_fd */)
#endif  // defined(OS_CHROMEOS)

// Requests the utility process to respond with a
// ChromeUtilityHostMsg_ProcessStarted message once it has started.  This may
// be used if the host process needs a handle to the running utility process.
IPC_MESSAGE_CONTROL0(ChromeUtilityMsg_StartupPing)

// Tells the utility process to analyze a zip file for malicious download
// protection.
IPC_MESSAGE_CONTROL1(ChromeUtilityMsg_AnalyzeZipFileForDownloadProtection,
                     IPC::PlatformFileForTransit /* zip_file */)

#if defined(OS_WIN)
// Tell the utility process to parse the iTunes preference XML file contents
// and return the path to the iTunes directory.
IPC_MESSAGE_CONTROL1(ChromeUtilityMsg_ParseITunesPrefXml,
                     std::string /* XML to parse */)
#endif  // defined(OS_WIN)

#if defined(OS_MACOSX)
// Tell the utility process to parse the iPhoto library XML file and
// return the parse result as well as the iPhoto library as an iphoto::Library.
IPC_MESSAGE_CONTROL1(ChromeUtilityMsg_ParseIPhotoLibraryXmlFile,
                     IPC::PlatformFileForTransit /* XML file to parse */)
#endif  // defined(OS_MACOSX)

#if defined(OS_WIN) || defined(OS_MACOSX)
// Tell the utility process to parse the iTunes library XML file and
// return the parse result as well as the iTunes library as an itunes::Library.
IPC_MESSAGE_CONTROL1(ChromeUtilityMsg_ParseITunesLibraryXmlFile,
                     IPC::PlatformFileForTransit /* XML file to parse */)

// Tells the utility process to parse the Picasa PMP database and return a
// listing of the user's Picasa albums and folders, along with metadata.
IPC_MESSAGE_CONTROL1(ChromeUtilityMsg_ParsePicasaPMPDatabase,
                     picasa::AlbumTableFilesForTransit /* album_table_files */)

// Tells the utility process to index the Picasa user-created Album contents
// by parsing all the INI files in Picasa Folders.
IPC_MESSAGE_CONTROL2(ChromeUtilityMsg_IndexPicasaAlbumsContents,
                     picasa::AlbumUIDSet /* album_uids */,
                     std::vector<picasa::FolderINIContents> /* folders_inis */)
#endif  // defined(OS_WIN) || defined(OS_MACOSX)

#if !defined(OS_ANDROID) && !defined(OS_IOS)
// Tell the utility process to attempt to validate the passed media file. The
// file will undergo basic sanity checks and will be decoded for up to
// |milliseconds_of_decoding| wall clock time. It is still not safe to decode
// the file in the browser process after this check.
IPC_MESSAGE_CONTROL2(ChromeUtilityMsg_CheckMediaFile,
                     int64 /* milliseconds_of_decoding */,
                     IPC::PlatformFileForTransit /* Media file to parse */)

IPC_MESSAGE_CONTROL3(ChromeUtilityMsg_ParseMediaMetadata,
                     std::string /* mime_type */,
                     int64 /* total_size */,
                     bool /* get_attached_images */)

IPC_MESSAGE_CONTROL2(ChromeUtilityMsg_RequestBlobBytes_Finished,
                     int64 /* request_id */,
                     std::string /* bytes */)
#endif  // !defined(OS_ANDROID) && !defined(OS_IOS)

// Requests that the utility process write the contents of the source file to
// the removable drive listed in the target file. The target will be restricted
// to removable drives by the utility process.
IPC_MESSAGE_CONTROL2(ChromeUtilityMsg_ImageWriter_Write,
                     base::FilePath /* source file */,
                     base::FilePath /* target file */)

// Requests that the utility process verify that the contents of the source file
// was written to the target. As above the target will be restricted to
// removable drives by the utility process.
IPC_MESSAGE_CONTROL2(ChromeUtilityMsg_ImageWriter_Verify,
                     base::FilePath /* source file */,
                     base::FilePath /* target file */)

// Cancels a pending write or verify operation.
IPC_MESSAGE_CONTROL0(ChromeUtilityMsg_ImageWriter_Cancel)

//------------------------------------------------------------------------------
// Utility process host messages:
// These are messages from the utility process to the browser.

// Reply when the utility process is done unpacking an extension.  |manifest|
// is the parsed manifest.json file.
// The unpacker should also have written out files containing the decoded
// images and message catalogs from the extension. The data is written into a
// DecodedImages struct into a file named kDecodedImagesFilename in the
// directory that was passed in. This is done because the data is too large to
// pass over IPC.
IPC_MESSAGE_CONTROL1(ChromeUtilityHostMsg_UnpackExtension_Succeeded,
                     base::DictionaryValue /* manifest */)

// Reply when the utility process has failed while unpacking an extension.
// |error_message| is a user-displayable explanation of what went wrong.
IPC_MESSAGE_CONTROL1(ChromeUtilityHostMsg_UnpackExtension_Failed,
                     base::string16 /* error_message, if any */)

// Reply when the utility process is done unpacking and parsing JSON data
// from a web resource.
IPC_MESSAGE_CONTROL1(ChromeUtilityHostMsg_UnpackWebResource_Succeeded,
                     base::DictionaryValue /* json data */)

// Reply when the utility process has failed while unpacking and parsing a
// web resource.  |error_message| is a user-readable explanation of what
// went wrong.
IPC_MESSAGE_CONTROL1(ChromeUtilityHostMsg_UnpackWebResource_Failed,
                     std::string /* error_message, if any */)

// Reply when the utility process has succeeded in parsing an update manifest
// xml document.
IPC_MESSAGE_CONTROL1(ChromeUtilityHostMsg_ParseUpdateManifest_Succeeded,
                     UpdateManifest::Results /* updates */)

// Reply when an error occurred parsing the update manifest. |error_message|
// is a description of what went wrong suitable for logging.
IPC_MESSAGE_CONTROL1(ChromeUtilityHostMsg_ParseUpdateManifest_Failed,
                     std::string /* error_message, if any */)

// Reply when the utility process has succeeded in decoding the image.
IPC_MESSAGE_CONTROL1(ChromeUtilityHostMsg_DecodeImage_Succeeded,
                     SkBitmap)  // decoded image

// Reply when an error occurred decoding the image.
IPC_MESSAGE_CONTROL0(ChromeUtilityHostMsg_DecodeImage_Failed)

// Reply when the utility process successfully parsed a JSON string.
//
// WARNING: The result can be of any Value subclass type, but we can't easily
// pass indeterminate value types by const object reference with our IPC macros,
// so we put the result Value into a ListValue. Handlers should examine the
// first (and only) element of the ListValue for the actual result.
IPC_MESSAGE_CONTROL1(ChromeUtilityHostMsg_ParseJSON_Succeeded,
                     base::ListValue)

// Reply when the utility process failed in parsing a JSON string.
IPC_MESSAGE_CONTROL1(ChromeUtilityHostMsg_ParseJSON_Failed,
                     std::string /* error message, if any*/)

// Reply when a file has been patched successfully.
IPC_MESSAGE_CONTROL0(ChromeUtilityHostMsg_PatchFile_Succeeded)

// Reply when patching a file failed.
IPC_MESSAGE_CONTROL1(ChromeUtilityHostMsg_PatchFile_Failed,
                     int /* error code */)

#if defined(OS_CHROMEOS)
// Reply when the utility process has succeeded in creating the zip file.
IPC_MESSAGE_CONTROL0(ChromeUtilityHostMsg_CreateZipFile_Succeeded)

// Reply when an error occured in creating the zip file.
IPC_MESSAGE_CONTROL0(ChromeUtilityHostMsg_CreateZipFile_Failed)
#endif  // defined(OS_CHROMEOS)

// Reply when the utility process has started.
IPC_MESSAGE_CONTROL0(ChromeUtilityHostMsg_ProcessStarted)

// Reply when a zip file has been analyzed for malicious download protection.
IPC_MESSAGE_CONTROL1(
    ChromeUtilityHostMsg_AnalyzeZipFileForDownloadProtection_Finished,
    safe_browsing::zip_analyzer::Results)

#if defined(OS_WIN)
// Reply after parsing the iTunes preferences XML file contents with either the
// path to the iTunes directory or an empty FilePath.
IPC_MESSAGE_CONTROL1(ChromeUtilityHostMsg_GotITunesDirectory,
                     base::FilePath /* Path to iTunes library */)
#endif  // defined(OS_WIN)

#if defined(OS_MACOSX)
// Reply after parsing the iPhoto library XML file with the parser result and
// an iphoto::Library data structure.
IPC_MESSAGE_CONTROL2(ChromeUtilityHostMsg_GotIPhotoLibrary,
                     bool /* Parser result */,
                     iphoto::parser::Library /* iPhoto library */)
#endif  // defined(OS_MACOSX)

#if defined(OS_WIN) || defined(OS_MACOSX)
// Reply after parsing the iTunes library XML file with the parser result and
// an itunes::Library data structure.
IPC_MESSAGE_CONTROL2(ChromeUtilityHostMsg_GotITunesLibrary,
                     bool /* Parser result */,
                     itunes::parser::Library /* iTunes library */)

// Reply after parsing the Picasa PMP Database with the parser result and a
// listing of the user's Picasa albums and folders, along with metadata.
IPC_MESSAGE_CONTROL3(ChromeUtilityHostMsg_ParsePicasaPMPDatabase_Finished,
                     bool /* parse_success */,
                     std::vector<picasa::AlbumInfo> /* albums */,
                     std::vector<picasa::AlbumInfo> /* folders */)

// Reply after indexing the Picasa user-created Album contents by parsing all
// the INI files in Picasa Folders.
IPC_MESSAGE_CONTROL1(ChromeUtilityHostMsg_IndexPicasaAlbumsContents_Finished,
                     picasa::AlbumImagesMap /* albums_images */)
#endif  // defined(OS_WIN) || defined(OS_MACOSX)

#if !defined(OS_ANDROID) && !defined(OS_IOS)
// Reply after checking the passed media file. A true result indicates that
// the file appears to be a well formed media file.
IPC_MESSAGE_CONTROL1(ChromeUtilityHostMsg_CheckMediaFile_Finished,
                     bool /* passed_checks */)

IPC_MESSAGE_CONTROL3(
    ChromeUtilityHostMsg_ParseMediaMetadata_Finished,
    bool /* parse_success */,
    base::DictionaryValue /* metadata */,
    std::vector<metadata::AttachedImage> /* attached_images */)

IPC_MESSAGE_CONTROL3(ChromeUtilityHostMsg_RequestBlobBytes,
                     int64 /* request_id */,
                     int64 /* start_byte */,
                     int64 /* length */)
#endif  // !defined(OS_ANDROID) && !defined(OS_IOS)

// Reply when a write or verify operation succeeds.
IPC_MESSAGE_CONTROL0(ChromeUtilityHostMsg_ImageWriter_Succeeded)

// Reply when a write or verify operation has been fully cancelled.
IPC_MESSAGE_CONTROL0(ChromeUtilityHostMsg_ImageWriter_Cancelled)

// Reply when a write or verify operation fails to complete.
IPC_MESSAGE_CONTROL1(ChromeUtilityHostMsg_ImageWriter_Failed,
                     std::string /* message */)

// Periodic status update about the progress of an operation.
IPC_MESSAGE_CONTROL1(ChromeUtilityHostMsg_ImageWriter_Progress,
                     int64 /* number of bytes processed */)

#if defined(OS_WIN)
// Get plain-text WiFi credentials from the system (requires UAC privilege
// elevation) and encrypt them with |public_key|.
IPC_MESSAGE_CONTROL2(ChromeUtilityHostMsg_GetAndEncryptWiFiCredentials,
                     std::string /* ssid */,
                     std::vector<uint8> /* public_key */)

// Reply after getting WiFi credentials from the system and encrypting them with
// caller's public key. |success| is false if error occurred.
IPC_MESSAGE_CONTROL2(ChromeUtilityHostMsg_GotEncryptedWiFiCredentials,
                     std::vector<uint8> /* encrypted_key_data */,
                     bool /* success */)
#endif  // defined(OS_WIN)
