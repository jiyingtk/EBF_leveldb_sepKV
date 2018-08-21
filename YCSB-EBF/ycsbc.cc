//
//  ycsbc.cc
//  YCSB-C
//
//  Created by Jinglei Ren on 12/19/14.
//  Copyright (c) 2014 Jinglei Ren <jinglei@ren.systems>.
//

#include <cstring>
#include <string>
#include <iostream>
#include <vector>
#include <future>
#include "core/utils.h"
#include "core/timer.h"
#include "core/client.h"
#include "core/core_workload.h"
#include "db/db_factory.h"
#include<boost/timer.hpp>
#include<memory>
using namespace std;

void UsageMessage(const char *command);
bool StrStartWith(const char *str, const char *pre);
string ParseCommandLine(int argc, const char *argv[], utils::Properties &props);
bool end_flag_ = false;
utils::Properties *props_ptr = NULL;

int thread_id = 0;

size_t ops[16] = {0};
unsigned long long durations[16] = {0};

size_t DelegateClient(ycsbc::DB *db, ycsbc::CoreWorkload *wl, const size_t num_ops,
                      bool is_loading, int read_thread_nums)
{
    int t_id = __sync_fetch_and_add(&thread_id, 1);

    int thread_type = read_thread_nums == 0 ? 0 : 1;
    if (thread_type && t_id >= read_thread_nums)
        thread_type = 2;

    FILE *fp_phase = NULL;
    db->Init();
    if (is_loading)
        fp_phase = fopen("phase_time.txt", "w");

    ycsbc::Client client(*db, *wl, t_id, thread_type);
    size_t oks = 0;

    int skipratio_inload = wl->skipratio_inload;
    cerr << "skipratio_inload" << skipratio_inload << endl;
    cerr << "num_ops" << num_ops << endl;
    
    struct timeval start_insert_time, end_insert_time, res_time;
    struct timeval start_phase_time, end_phase_time;
    gettimeofday(&start_insert_time, NULL);
    gettimeofday(&start_phase_time, NULL);
    for (size_t i = 0; i < num_ops; ++i)
    {
        if (is_loading)
        {
            if(skipratio_inload && i % skipratio_inload != 0)
            {
                client.DoInsert(false);
                continue;
            }
            oks += client.DoInsert();
        }
        else
        {
            oks += client.DoTransaction(ops, durations);
        }
        if(i % 10240 == 0)
        {
            if(is_loading && i % 102400 == 0)
            {
                cerr << "operation count:" << i << "\r";
                gettimeofday(&end_phase_time, NULL);
                timersub(&end_phase_time, &start_phase_time, &res_time);
                start_phase_time = end_phase_time;
                fprintf(fp_phase, "%lu\n", res_time.tv_sec * 1000000 + res_time.tv_usec);
            }
            else if(!is_loading)
            {
                cerr << "operation count:" << i << "\r";
            }
        }
        if (i % 100000 == 0)
        {
            db->doSomeThing("printFP");
            db->doSomeThing("printAccessFreq");
        }
    }
    gettimeofday(&end_insert_time, NULL);
    timersub(&end_insert_time, &start_insert_time, &res_time);
    cout << endl;
    if(!is_loading)
    {
        unsigned long long real_used_time = res_time.tv_sec * 1000000 + res_time.tv_usec;
        if (read_thread_nums == 0)
            cout << "Thread type: " << "mixed read and write" << endl;
        else
            cout << "Thread type: " << (thread_type == 1 ? "only READ" : "only WRITE") << endl;
        cout << "Used time: " << durations[ycsbc::Operation::THREAD0 + t_id] / 1000000.0 << "s" << endl;
        cout << "WRITE latency " << endl;
        if (ops[ycsbc::Operation::INSERT])
            cout << durations[ycsbc::Operation::INSERT] / ops[ycsbc::Operation::INSERT] << "us" << " Write ops: " << ops[ycsbc::Operation::INSERT] << endl;
        cout << "READ latency(including zero-result lookup)" << endl;
        if (ops[ycsbc::Operation::READ])
            cout << durations[ycsbc::Operation::READ] / ops[ycsbc::Operation::READ] << "us" << " Read ops: " << ops[ycsbc::Operation::READ] << endl;
        cout << "Zero-result lookup: " << endl;
        if (ops[ycsbc::Operation::ZEROREAD])
            cout << durations[ycsbc::Operation::ZEROREAD] / ops[ycsbc::Operation::ZEROREAD] << "us" << " Zero-result ops: " << ops[ycsbc::Operation::ZEROREAD] << endl;
        cout << "Total IOPS: " << endl;
        cout << ops[ycsbc::Operation::ALL] / (durations[ycsbc::Operation::THREAD0 + t_id] / 1000000.0) << endl;
        cout << "Read IOPS: " << endl;
        cout << ops[ycsbc::Operation::READ] / (durations[ycsbc::Operation::THREAD0 + t_id] / 1000000.0) << ", " << ops[ycsbc::Operation::READ] / (real_used_time / 1000000.0) << endl;
        // cout<<ops[ycsbc::Operation::READ]/(durations[ycsbc::Operation::READ]/1000000.0)<<endl;
        cout << "WRITE IOPS: " << endl;
        cout << ops[ycsbc::Operation::INSERT] / (durations[ycsbc::Operation::THREAD0 + t_id] / 1000000.0) << ", " << ops[ycsbc::Operation::INSERT] / (real_used_time / 1000000.0) << endl;
        // cout<<ops[ycsbc::Operation::INSERT]/(durations[ycsbc::Operation::INSERT]/1000000.0)<<endl;

        db->doSomeThing("printStats");

        db->doSomeThing("printAccessFreq");
    }
    else
    {
        cout << "Total time of insert: " << res_time.tv_sec * 1000000 + res_time.tv_usec << "us" << endl;
        cerr << "oks: " << oks << endl;
        cout << "Per insert time: " << (res_time.tv_sec * 1000000 + res_time.tv_usec) * 1.0 / oks << "us" << endl;
    }

    if(fp_phase != NULL)
    {
        fclose(fp_phase);
        fp_phase = NULL;
    }
    return oks;
}

int main(const int argc, const char *argv[])
{
    utils::Properties props;
    string file_name = ParseCommandLine(argc, argv, props);
    struct timeval start_main_time, end_main_time, res_main_time;
    ycsbc::DB *db = ycsbc::DBFactory::CreateDB(props);
    if (!db)
    {
        cout << "Unknown database name " << props["dbname"] << endl;
        exit(0);
    }


    props_ptr = &props;
    const size_t num_threads = static_cast<size_t>(stoi(props.GetProperty("threadcount", "1")));
    bool skipLoad = utils::StrToBool(props.GetProperty("skipLoad",
                                     "false"));
    // Loads data

    vector<future<size_t>> actual_ops;
    size_t total_ops = 0;
    sscanf(props[ycsbc::CoreWorkload::RECORD_COUNT_PROPERTY].c_str(), "%zu", &total_ops);
    size_t sum = 0;
    gettimeofday(&start_main_time, NULL);
    if(!skipLoad)
    {
        ycsbc::CoreWorkload wl;
        wl.Init(props);

        ycsbc::WallTimer loadRunTimer;
        loadRunTimer.Start();
        for (size_t i = 0; i < num_threads; ++i)
        {
            actual_ops.emplace_back(async(launch::async,
                                          DelegateClient, db, &wl, total_ops / num_threads, true, 0));
        }
        assert(actual_ops.size() == num_threads);
        for (auto &n : actual_ops)
        {
            assert(n.valid());
            sum += n.get();
        }
        cerr << "# Loading records:\t" << sum << endl;
        cerr << "load time: " << loadRunTimer.elapsed() << "us" << endl;

        //loadRunTimer.restart();
        // Peforms transactions
        actual_ops.clear();
    }

    total_ops = stoi(props[ycsbc::CoreWorkload::OPERATION_COUNT_PROPERTY]);
    ycsbc::WallTimer timer;
    db->openStatistics();
    int read_thread_nums = std::stoi(props.GetProperty("readTheadNums", "0"));

    int runs;
    for (runs = 0; runs < 2; runs++) {
        if (runs == 0)
            cerr << "First run workload\n" << endl;
        else {
            cerr << "Second run workload (adjust filter)\n" << endl;
            memset(ops, 0, sizeof(size_t) * 16);
            memset(durations, 0, sizeof(unsigned long long) * 16);
        }

        ycsbc::CoreWorkload wl;
        wl.Init(props);

        timer.Start();
        for (size_t i = 0; i < num_threads; ++i)
        {
            actual_ops.emplace_back(async(launch::async,
                                          DelegateClient, db, &wl, total_ops / num_threads, false, read_thread_nums));
        }
        assert(actual_ops.size() == num_threads);

        sum = 0;
        for (auto &n : actual_ops)
        {
            assert(n.valid());
            sum += n.get();
        }
        db->doSomeThing("printFP");

        double duration = timer.elapsed();
        cerr << endl;
        cerr << "# Transaction throughput (KTPS)" << endl;
        cerr << total_ops / (duration / 1000000) / 1000 << endl;
        cerr << props["dbname"] << '\t' << file_name << '\t' << num_threads << '\t' << endl;;
        cerr << "run time: " << timer.elapsed() << "us" << endl;
        gettimeofday(&end_main_time, NULL);
        timersub(&end_main_time, &start_main_time, &res_main_time);
        cout << "Total time of main: " << res_main_time.tv_sec * 1000000 + res_main_time.tv_usec << "us" << endl;
        cout << "\n\n" << endl;

        if(!wl.adjust_filter_)
            runs++;
        else if (runs == 0)
            cout << "Adjust bloom filter accroding to access frequencies" << endl;
        actual_ops.clear();
    }

    db->Close();
    delete db;
}

string ParseCommandLine(int argc, const char *argv[], utils::Properties &props)
{
    int argindex = 1;
    string filename;
    string latency_filename;
    while (argindex < argc && StrStartWith(argv[argindex], "-"))
    {
        if (strcmp(argv[argindex], "-threads") == 0)
        {
            argindex++;
            if (argindex >= argc)
            {
                UsageMessage(argv[0]);
                exit(0);
            }
            props.SetProperty("threadcount", argv[argindex]);
            argindex++;
        }
        else if (strcmp(argv[argindex], "-db") == 0)
        {
            argindex++;
            if (argindex >= argc)
            {
                UsageMessage(argv[0]);
                exit(0);
            }
            props.SetProperty("dbname", argv[argindex]);
            argindex++;
        }
        else if (strcmp(argv[argindex], "-host") == 0)
        {
            argindex++;
            if (argindex >= argc)
            {
                UsageMessage(argv[0]);
                exit(0);
            }
            props.SetProperty("host", argv[argindex]);
            argindex++;
        }
        else if (strcmp(argv[argindex], "-port") == 0)
        {
            argindex++;
            if (argindex >= argc)
            {
                UsageMessage(argv[0]);
                exit(0);
            }
            props.SetProperty("port", argv[argindex]);
            argindex++;
        }
        else if (strcmp(argv[argindex], "-slaves") == 0)
        {
            argindex++;
            if (argindex >= argc)
            {
                UsageMessage(argv[0]);
                exit(0);
            }
            props.SetProperty("slaves", argv[argindex]);
            argindex++;
        }
        else if (strcmp(argv[argindex], "-P") == 0)
        {
            argindex++;
            if (argindex >= argc)
            {
                UsageMessage(argv[0]);
                exit(0);
            }
            filename.assign(argv[argindex]);
            ifstream input(argv[argindex]);
            try
            {
                props.Load(input);
            }
            catch (const string &message)
            {
                cout << message << endl;
                exit(0);
            }
            input.close();
            argindex++;
        }
        else if(strcmp(argv[argindex], "-dbfilename") == 0)
        {
            argindex++;
            if(argindex >= argc)
            {
                UsageMessage(argv[0]);
                exit(0);
            }
            props.SetProperty("dbfilename", argv[argindex]);
            argindex++;
        }
        else if(strcmp(argv[argindex], "-configpath") == 0)
        {
            argindex++;
            if(argindex >= argc)
            {
                UsageMessage(argv[0]);
                exit(0);
            }
            props.SetProperty("configpath", argv[argindex]);
            argindex++;
        }
        else if(strcmp(argv[argindex], "-skipLoad") == 0)
        {
            argindex++;
            if(argindex >= argc)
            {
                UsageMessage(argv[0]);
                exit(0);
            }
            props.SetProperty("skipLoad", argv[argindex]);
            argindex++;
        }
        else if(strcmp(argv[argindex], "-requestdistribution") == 0)
        {
            argindex++;
            if(argindex >= argc)
            {
                UsageMessage(argv[0]);
                exit(0);
            }
            props.SetProperty("requestdistribution", argv[argindex]);
            argindex++;
        }
        else if(strcmp(argv[argindex], "-zipfianconst") == 0)
        {
            argindex++;
            if(argindex >= argc)
            {
                UsageMessage(argv[0]);
                exit(0);
            }
            props.SetProperty("zipfianconst", argv[argindex]);
            argindex++;
        }
        else if (strcmp(argv[argindex], "-readTheadNums") == 0)
        {
            argindex++;
            if(argindex >= argc)
            {
                UsageMessage(argv[0]);
                exit(0);
            }
            props.SetProperty("readTheadNums", argv[argindex]);
            argindex++;
        }
        else
        {
            cout << "Unknown option '" << argv[argindex] << "'" << endl;
            exit(0);
        }
    }

    if (argindex == 1 || argindex != argc)
    {
        UsageMessage(argv[0]);
        exit(0);
    }

    return filename;
}

void UsageMessage(const char *command)
{
    cout << "Usage: " << command << " [options]" << endl;
    cout << "Options:" << endl;
    cout << "  -threads n: execute using n threads (default: 1)" << endl;
    cout << "  -db dbname: specify the name of the DB to use (default: basic)" << endl;
    cout << "  -P propertyfile: load properties from the given file. Multiple files can" << endl;
    cout << "                   be specified, and will be processed in the order specified" << endl;
}

inline bool StrStartWith(const char *str, const char *pre)
{
    return strncmp(str, pre, strlen(pre)) == 0;
}

