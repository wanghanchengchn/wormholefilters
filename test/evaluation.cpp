#include "assert.h"
#include "pm_wf/pmwormholefilter.hpp"

#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <libpmemobj.h>
#include <openssl/rand.h>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>

using namespace std;

#define POOL_SIZE 512 * 1024 * 1024

::std::uint64_t NowNanos()
{
    return ::std::chrono::duration_cast<::std::chrono::nanoseconds>(
               ::std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

int main()
{
    uint64_t *vals;
    uint64_t nvals = 1024 * 1024 * 8;

    vals = (uint64_t *)malloc(nvals * sizeof(vals[0]));
    RAND_bytes((unsigned char *)vals, sizeof(*vals) * nvals);
    srand(0);

    char file_name[1000] = "/mnt/pmem00/pmwormholefilter.pool";

    PMEMobjpool *pop;
    if (access(file_name, F_OK))
    {
        if ((pop = pmemobj_create(file_name, POBJ_LAYOUT_NAME(pmwormholefilter_root), POOL_SIZE, 0666)) == NULL)
        {
            fprintf(stderr, "%s", pmemobj_errormsg());
            exit(1);
        }
    }
    else
    {
        if (remove(file_name) == 0)
        {
            printf("remove successfully\n");
        }
        if ((pop = pmemobj_create(file_name, POBJ_LAYOUT_NAME(pmwormholefilter_root), POOL_SIZE, 0666)) == NULL)
        {
            fprintf(stderr, "%s", pmemobj_errormsg());
            exit(1);
        }
    }

    TOID(struct pmwormholefilter_root)
    pmwormholefilter_root = POBJ_ROOT(pop, struct pmwormholefilter_root);

    pmwormholefilter_init(pop, pmwormholefilter_root, nvals);

    uint64_t added = 0;
    auto start_time = NowNanos();
    for (added = 0; added < nvals; added++)
    {
        if (pmwormholefilter_insert(pop, pmwormholefilter_root, vals[added]) == false)
        {
            cout << "Full" << endl;
            break;
        }
    }
    cout << "Insertion throughput: " << 1000.0 * added / static_cast<double>(NowNanos() - start_time) << " MOPS" << endl;

    start_time = NowNanos();
    for (int looked = 0; looked < added; looked++)
    {
        if (pmwormholefilter_lookup(pop, pmwormholefilter_root, vals[looked]) == false)
        {
            cout << "ERROR" << endl;
        }
    }
    cout << "Lookup throughput: " << 1000.0 * added / static_cast<double>(NowNanos() - start_time) << " MOPS" << endl;

    cout << "PASS" << endl;

    return 0;
}