/***************************************************************************
 * 
 * Copyright (c) 2018 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file demo.cpp
 * @author wanlijin01(wanlijin01@baidu.com)
 * @date 2018/07/09 20:12:44
 * @brief 
 *  
 **/
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common.h"
#include <fstream>
#include "predictor_sdk.h"
#include "echo_service.pb.h"
#include "builtin_format.pb.h"

using baidu::paddle_serving::sdk_cpp::Predictor;
using baidu::paddle_serving::sdk_cpp::PredictorApi;
using baidu::paddle_serving::predictor::echo_service::RequestAndResponse;

int create_req(RequestAndResponse& req) {
    req.set_a(1);
    req.set_b(0.1);
    return 0;
}

void print_res(
        const RequestAndResponse& req,
        const RequestAndResponse& res,
        std::string route_tag,
        uint64_t elapse_ms) {
    LOG(INFO) << "Reqeive result: a = " << res.a() << ", b = " << res.b();

    LOG(INFO) 
        << "Succ call predictor[echo_service], the tag is: " 
        << route_tag << ", elapse_ms: " << elapse_ms;
}

int main(int argc, char** argv) {
    PredictorApi api;

    // initialize logger instance
    struct stat st_buf;
    int ret = 0;
    if ((ret = stat("./log", &st_buf)) != 0) {
            mkdir("./log", 0777);
            ret = stat("./log", &st_buf);
            if (ret != 0) {
                    LOG(WARNING) << "Log path ./log not exist, and create fail";
                    return -1;
            }
    }
    FLAGS_log_dir = "./log";
    google::InitGoogleLogging(strdup(argv[0]));
     
    if (api.create("./conf", "predictors.prototxt") != 0) {
        LOG(ERROR) << "Failed create predictors api!"; 
        return -1;
    }

    RequestAndResponse req;
    RequestAndResponse res;

    api.thrd_initialize();

    while (true) {
        timeval start;
        gettimeofday(&start, NULL);

        api.thrd_clear();

        Predictor* predictor = api.fetch_predictor("echo_service");
        if (!predictor) {
            LOG(ERROR) << "Failed fetch predictor: echo_service"; 
            return -1;
        }

        req.Clear();
        res.Clear();

        if (create_req(req) != 0) {
            return -1;
        }

        butil::IOBufBuilder debug_os;
        if (predictor->debug(&req, &res, &debug_os) != 0) {
            LOG(ERROR) << "failed call predictor with req:"
                        << req.ShortDebugString();
            return -1;
        }

        butil::IOBuf debug_buf;
        debug_os.move_to(debug_buf);
        LOG(INFO) << "Debug string: " << debug_buf;

        timeval end;
        gettimeofday(&end, NULL);

        uint64_t elapse_ms = (end.tv_sec * 1000 + end.tv_usec / 1000)
            - (start.tv_sec * 1000 + start.tv_usec / 1000);
    
        print_res(req, res, predictor->tag(), elapse_ms);
        res.Clear();

        usleep(50);

    } // while (true)

    api.thrd_finalize();
    api.destroy();

    google::ShutdownGoogleLogging();

    return 0;
}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */