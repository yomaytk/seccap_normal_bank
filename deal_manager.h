#ifndef _DEAL_MANAGER_H_
#define _DEAL_MANAGER_H_

#include <vector>
#include "account.h"

namespace AS = AccountSpace;

namespace Manager {
    class DealManager {
       public:
        AS::Account* current_account;
        std::vector<AS::Account*> account_list;

        bool AccountLogin();
        ProcessResult SetCurrentAccount(AS::Account* account);
        uint64_t DealMyDeposit(uint64_t amount);
        uint64_t DealMyWithdraw(uint64_t amount);
        ProcessResult NewAccount(const char* name, const char* password);
        ProcessResult AccountListStoreStorage();
        ProcessResult AccountListSetting();
        DealManager();
        ~DealManager();
    };
};  // namespace Manager

#endif