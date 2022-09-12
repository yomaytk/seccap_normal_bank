#include "server.h"
#include <assert.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>
#include <cstring>
#include <ext/stdio_filebuf.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include "deal_manager.h"

const int RECEIVE_BUF_SIZE = 1024;
const std::string PROJECT_PATH = "/opt/intel/sgxsdk/SampleCode/seccap_normal_bank/";
// const std::string PROJECT_PATH = "/home/masashi/workspace/seccap/seccap_normal_bank/";

void account_list_store(uint8_t* account_list_data, uint64_t account_data_len) {
    std::ofstream ofs((PROJECT_PATH + "account").c_str(),
                      std::ios::binary | std::ios::out | std::ios::in);

    if (!ofs.good()) {
        printf("write open account file failed.\n");
        exit(EXIT_FAILURE);
    }

    ofs.write(reinterpret_cast<const char*>(account_list_data), account_data_len);

    if (ofs.fail()) {
        printf("write execute account file failed.\n");
        exit(EXIT_FAILURE);
    }

    ofs.close();
}

uint64_t get_account_list_size() {
    FILE* fp = fopen((PROJECT_PATH + "account").c_str(), "rb");

    if (fp == NULL) {
        printf("read account failed.\n");
        exit(EXIT_FAILURE);
    }

    if (fseek(fp, 0L, SEEK_END) == 0) {
        fpos_t pos;

        if (fgetpos(fp, &pos) == 0) {
            fclose(fp);
            return (uint64_t)pos.__pos;
        }
    }

    return -1;
}

void get_account_list(uint8_t* account_list_data, uint64_t account_list_size) {
    if (account_list_size > 0) {
        std::ifstream ifs((PROJECT_PATH + "account").c_str(), std::ios::binary | std::ios::in);
        if (ifs.is_open()) {
            ifs.read(reinterpret_cast<char*>(account_list_data), account_list_size);
            if (ifs.fail()) {
                printf("read account file failed.\n");
            }
            ifs.close();
            // printf("get account list end.\n");
        } else {
            printf("open account file failed.\n");
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char* argv[]) {
    (void)(argc);
    (void)(argv);

    enum ProcessResult result;

    int sockfd;
    int client_sockfd;
    struct sockaddr_in addr;

    socklen_t len = sizeof(struct sockaddr_in);
    struct sockaddr_in from_addr;

    char buf[RECEIVE_BUF_SIZE];

    // 受信バッファ初期化
    memset(buf, 0, sizeof(buf));

    // ソケット生成
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
    }

    uint16_t port = 1230;
    char addr_num[9];
    port += atoi(argv[1]);

    // 待ち受け用IP・ポート番号設定
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    // バインド
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
    }

    // 受信待ち
    if (listen(sockfd, SOMAXCONN) < 0) {
        perror("listen");
    }

    auto deal_manager = new Manager::DealManager();
    deal_manager->AccountListSetting();
    printf("[Server]: サーバー初期設定完了\n");

    // クライアントからのコネクト要求待ち
    if ((client_sockfd = accept(sockfd, (struct sockaddr*)&from_addr, &len)) < 0) {
        perror("accept");
    }

    // 受信
    int rsize;
    for (;;) {
        for (;;) {
            rsize = recv(client_sockfd, buf, sizeof(buf), 0);

            if (rsize == 0) {
                break;
            } else if (rsize == -1) {
                perror("recv");
            } else {
                char* data[3];
                int data_size[3];
                int data_iter  = 0;
                int start_iter = 0;
                for (int i = 0; i < RECEIVE_BUF_SIZE; i++) {
                    if (buf[i] == ',') {
                        int end_iter         = i;
                        data_size[data_iter] = end_iter - start_iter;
                        data[data_iter]      = (char*)calloc(1, data_size[data_iter] + 1);
                        std::memcpy(data[data_iter], buf + start_iter, data_size[data_iter]);
                        start_iter = i + 1;
                        data_iter++;
                    }
                }
                if (strncmp("login", data[0], 5) == 0) {
                    std::cout << "[Server]: ログイン処理スタート\n";
                    ProcessResult result = deal_manager->NewAccount(data[1], data[2]);
                    if (result == ProcessResult::ACCOUNT_SET_FAIL) {
                        write(client_sockfd, "1", 1);
                    } else {
                        write(client_sockfd, "0", 1);
                    }
                } else if (strncmp("deposit", data[0], 7) == 0) {
                    std::cout << "[Server]: 預金処理スタート\n";
                    uint64_t amount   = atoi(data[1]);
                    uint64_t deposits = deal_manager->DealMyDeposit(amount);
                    char send_data[8];
                    sprintf(send_data, "%ld", deposits);
                    write(client_sockfd, send_data, sizeof(uint64_t));
                } else if (strncmp("withdraw", data[0], 8) == 0) {
                    std::cout << "[Server]: 引き出し処理スタート\n";
                    uint64_t amount   = atoi(data[1]);
                    uint64_t deposits = deal_manager->DealMyWithdraw(amount);
                    char send_data[8];
                    sprintf(send_data, "%ld", deposits);
                    write(client_sockfd, send_data, sizeof(uint64_t));
                } else if (strncmp("end", buf, 3) == 0) {
                    goto server_end;
                }
                std::cout << "[Server]: 完了\n";
            }
        }
    }

server_end:

    // ソケットクローズ
    close(client_sockfd);
    close(sockfd);

    return 0;
}
