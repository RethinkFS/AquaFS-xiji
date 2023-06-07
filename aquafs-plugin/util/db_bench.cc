// //  Copyright (c) 2013-present, Facebook, Inc.  All rights reserved.
// //  This source code is licensed under both the GPLv2 (found in the
// //  COPYING file in the root directory) and Apache 2.0 License
// //  (found in the LICENSE.Apache file in the root directory).
// //
// // Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// // Use of this source code is governed by a BSD-style license that can be
// // found in the LICENSE file. See the AUTHORS file for names of contributors.
// #include <cstdio>
// #include <string>
//
// #ifndef GFLAGS
// #include <cstdio>
// int main() {
//   fprintf(stderr, "Please install gflags to run rocksdb tools\n");
//   return 1;
// }
// #else
// #include <gflags/gflags.h>
// #include <rocksdb/convenience.h>
// #include <rocksdb/env.h>
// #include <rocksdb/file_system.h>
//
// #include "fs/fs_aquafs.h"
// #include "port/stack_trace.h"
// #include "rocksdb/db_bench_tool.h"
//
// using namespace rocksdb;
// using namespace google;
//
// // int db_bench_tool(int argc, char** argv) {
// //   ROCKSDB_NAMESPACE::port::InstallStackTraceHandler();
// //   ConfigOptions config_options;
// //   static bool initialized = false;
// //   if (!initialized) {
// //     SetUsageMessage(std::string("\nUSAGE:\n") + std::string(argv[0]) +
// //                     " [OPTIONS]...");
// //     SetVersionString(GetRocksVersionAsString(true));
// //     initialized = true;
// //   }
// //   ParseCommandLineFlags(&argc, &argv, true);
// // //   FLAGS_compaction_style_e =
// // //       (ROCKSDB_NAMESPACE::CompactionStyle)FLAGS_compaction_style;
// // //   if (FLAGS_statistics && !FLAGS_statistics_string.empty()) {
// // //     fprintf(stderr,
// // //             "Cannot provide both --statistics and
// --statistics_string.\n");
// // //     exit(1);
// // //   }
// // //   if (!FLAGS_statistics_string.empty()) {
// // //     Status s = Statistics::CreateFromString(config_options,
// // //                                             FLAGS_statistics_string,
// &dbstats);
// // //     if (dbstats == nullptr) {
// // //       fprintf(stderr,
// // //               "No Statistics registered matching string: %s
// status=%s\n",
// // //               FLAGS_statistics_string.c_str(), s.ToString().c_str());
// // //       exit(1);
// // //     }
// // //   }
// // //   if (FLAGS_statistics) {
// // //     dbstats = ROCKSDB_NAMESPACE::CreateDBStatistics();
// // //   }
// // //   if (dbstats) {
// // // dbstats->set_stats_level(static_cast<StatsLevel>(FLAGS_stats_level));
// // //   }
// // //   FLAGS_compaction_pri_e =
// // //       (ROCKSDB_NAMESPACE::CompactionPri)FLAGS_compaction_pri;
// // //
// // //   std::vector<std::string> fanout = ROCKSDB_NAMESPACE::StringSplit(
// // //       FLAGS_max_bytes_for_level_multiplier_additional, ',');
// // //   for (size_t j = 0; j < fanout.size(); j++) {
// // //     FLAGS_max_bytes_for_level_multiplier_additional_v.push_back(
// // // #ifndef CYGWIN
// // //         std::stoi(fanout[j]));
// // // #else
// // //         stoi(fanout[j]));
// // // #endif
// // //   }
// // //
// // //   FLAGS_compression_type_e =
// // //       StringToCompressionType(FLAGS_compression_type.c_str());
// // //
// // //   FLAGS_wal_compression_e =
// // //       StringToCompressionType(FLAGS_wal_compression.c_str());
// // //
// // //   FLAGS_compressed_secondary_cache_compression_type_e =
// StringToCompressionType(
// // //       FLAGS_compressed_secondary_cache_compression_type.c_str());
// // //
// // //   // Stacked BlobDB
// // //   FLAGS_blob_db_compression_type_e =
// // //       StringToCompressionType(FLAGS_blob_db_compression_type.c_str());
// // //
// // //   int env_opts = !FLAGS_env_uri.empty() + !FLAGS_fs_uri.empty();
// // //   if (env_opts > 1) {
// // //     fprintf(stderr, "Error: --env_uri and --fs_uri are mutually
// exclusive\n");
// // //     exit(1);
// // //   }
// //
// //   // if (env_opts == 1) {
// //     Status s = Env::CreateFromUri(config_options, "",
// "aquafs://dev:nullb0",
// //                                   &FLAGS_env, &env_guard);
// //     if (!s.ok()) {
// //       fprintf(stderr, "Failed creating env: %s\n", s.ToString().c_str());
// //       exit(1);
// //     }
// //   // } else if (FLAGS_simulate_hdd || FLAGS_simulate_hybrid_fs_file != "")
// {
// //   //   //**TODO: Make the simulate fs something that can be loaded
// //   //   // from the ObjectRegistry...
// //   //   static std::shared_ptr<ROCKSDB_NAMESPACE::Env> composite_env =
// //   //       NewCompositeEnv(std::make_shared<SimulatedHybridFileSystem>(
// //   //           FileSystem::Default(), FLAGS_simulate_hybrid_fs_file,
// //   //           /*throughput_multiplier=*/
// //   //           int{FLAGS_simulate_hybrid_hdd_multipliers},
// //   //           /*is_full_fs_warm=*/FLAGS_simulate_hdd));
// //   //   FLAGS_env = composite_env.get();
// //   // }
// //
// //   // Let -readonly imply -use_existing_db
// //   // FLAGS_use_existing_db |= FLAGS_readonly;
// //
// //   // if (FLAGS_build_info) {
// //   //   std::string build_info;
// //   //   std::cout << GetRocksBuildInfoAsString(build_info, true) <<
// std::endl;
// //   //   // Similar to --version, nothing else will be done when this flag
// is set
// //   //   exit(0);
// //   // }
// //
// //   // if (!FLAGS_seed) {
// //   //   uint64_t now = FLAGS_env->GetSystemClock()->NowMicros();
// //   //   seed_base = static_cast<int64_t>(now);
// //   //   fprintf(stdout, "Set seed to %" PRIu64 " because --seed was 0\n",
// //   //           seed_base);
// //   // } else {
// //   //   seed_base = FLAGS_seed;
// //   // }
// //
// //   // if (FLAGS_use_existing_keys && !FLAGS_use_existing_db) {
// //   //   fprintf(stderr,
// //   //           "`-use_existing_db` must be true for `-use_existing_keys`
// to be "
// //   //           "settable\n");
// //   //   exit(1);
// //   // }
// //
// //   // if (!strcasecmp(FLAGS_compaction_fadvice.c_str(), "NONE"))
// //   //   FLAGS_compaction_fadvice_e = ROCKSDB_NAMESPACE::Options::NONE;
// //   // else if (!strcasecmp(FLAGS_compaction_fadvice.c_str(), "NORMAL"))
// //   //   FLAGS_compaction_fadvice_e = ROCKSDB_NAMESPACE::Options::NORMAL;
// //   // else if (!strcasecmp(FLAGS_compaction_fadvice.c_str(), "SEQUENTIAL"))
// //   //   FLAGS_compaction_fadvice_e =
// ROCKSDB_NAMESPACE::Options::SEQUENTIAL;
// //   // else if (!strcasecmp(FLAGS_compaction_fadvice.c_str(), "WILLNEED"))
// //   //   FLAGS_compaction_fadvice_e = ROCKSDB_NAMESPACE::Options::WILLNEED;
// //   // else {
// //   //   fprintf(stdout, "Unknown compaction fadvice:%s\n",
// //   //           FLAGS_compaction_fadvice.c_str());
// //   //   exit(1);
// //   // }
// //
// //   // FLAGS_value_size_distribution_type_e =
// //   // StringToDistributionType(FLAGS_value_size_distribution_type.c_str());
// //
// //   // Note options sanitization may increase thread pool sizes according to
// //   // max_background_flushes/max_background_compactions/max_background_jobs
// //   // FLAGS_env->SetBackgroundThreads(FLAGS_num_high_pri_threads,
// //   // ROCKSDB_NAMESPACE::Env::Priority::HIGH);
// //   // FLAGS_env->SetBackgroundThreads(FLAGS_num_bottom_pri_threads,
// //   // ROCKSDB_NAMESPACE::Env::Priority::BOTTOM);
// //   // FLAGS_env->SetBackgroundThreads(FLAGS_num_low_pri_threads,
// //   // ROCKSDB_NAMESPACE::Env::Priority::LOW);
// //
// //   // Choose a location for the test database if none given with
// --db=<path>
// //   // if (FLAGS_db.empty()) {
// //     std::string default_db_path;
// //     FLAGS_env->GetTestDirectory(&default_db_path);
// //     default_db_path += "/dbbench";
// //     // FLAGS_db = default_db_path;
// //   // }
// //
// //   // if (FLAGS_backup_dir.empty()) {
// //   //   FLAGS_backup_dir = FLAGS_db + "/backup";
// //   // }
// //   //
// //   // if (FLAGS_restore_dir.empty()) {
// //   //   FLAGS_restore_dir = FLAGS_db + "/restore";
// //   // }
// //
// //   // if (FLAGS_stats_interval_seconds > 0) {
// //   //   // When both are set then FLAGS_stats_interval determines the
// frequency
// //   //   // at which the timer is checked for FLAGS_stats_interval_seconds
// //   //   FLAGS_stats_interval = 1000;
// //   // }
// //
// //   // if (FLAGS_seek_missing_prefix && FLAGS_prefix_size <= 8) {
// //   //   fprintf(stderr, "prefix_size > 8 required by
// --seek_missing_prefix\n");
// //   //   exit(1);
// //   // }
// //
// //   // ROCKSDB_NAMESPACE::Benchmark benchmark;
// //   // benchmark.Run();
// //
// //   // if (FLAGS_print_malloc_stats) {
// //   //   std::string stats_string;
// //   //   ROCKSDB_NAMESPACE::DumpMallocStats(&stats_string);
// //   //   fprintf(stdout, "Malloc stats:\n%s\n", stats_string.c_str());
// //   // }
// //
// //   return 0;
// // }
//
// int main(int argc, char** argv) {
//   assert(FLAGS_env != nullptr);
//   // return ROCKSDB_NAMESPACE::db_bench_tool(argc, argv);
//
//   ROCKSDB_NAMESPACE::port::InstallStackTraceHandler();
//   SetUsageMessage(std::string("\nUSAGE:\n") + std::string(argv[0]) +
//                   " [OPTIONS]...");
//   SetVersionString(GetRocksVersionAsString(true));
//   ParseCommandLineFlags(&argc, &argv, true);
//   ConfigOptions config_options;
//   // std::shared_ptr<FileSystem> fs;
//   FileSystem* fs;
//   std::string fs_uri = "aquafs://dev:nullb0";
//   // std::string fs_uri = "";
//   // // Status s = Env::CreateFromUri(config_options, "", fs_uri, &FLAGS_env,
//   // &env_guard); Status s = FileSystem::CreateFromString(config_options,
//   // fs_uri, &fs); if (!s.ok()) {
//   //   printf("%s\n", s.getState());
//   // } else {
//   //   printf("got fs: %s\n", fs->kDefaultName());
//   // }
//   // assert(s.ok());
//
//   // fs = make_shared(new AquaFS());
//   Status s = NewAquaFS(&fs, ZbdBackendType::kBlockDev, "nullb0");
//   if (!s.ok()) {
//     printf("%s\n", s.getState());
//   }
//   // assert(s.ok());
//   printf("done.\n");
//   return 0;
// }
// #endif  // GFLAGS

int main() { return 0; }