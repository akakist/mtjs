#include <quickjs_i.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <set>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <optional>
#include <dirent.h>
#include <sys/stat.h>

#include <nlohmann/json.hpp>

#include "REF.h"
#include "jsiValueGuard.h"
#include "jsiscope.h"
#include "GranuleIO.h"
#include "jsiMsg.h"
#include "CallFrame.h"
#include "CallStack.h"
#include "CallContext.h"
#include "ContractContext.h"
#include "contract_constants.h"
#include "Granule.h"
#include "EventSink.h"
#include "jsiEvent.h"
#include "Clock.h"
#include "ContractSourceIO.h"
#include "TestGranuleIO.h"
#include "ContractHost.h"
#include "convert.h"
using json = nlohmann::json;


struct ContractContext;
struct ContractHost;
struct GranuleIO;
struct EventSink;
struct Clock;
struct ContractSourceIO;

JSIValue build_msg_object(JSIContext* ctx, const jsiMsg& m);

static REF_getter<ContractHost> make_test() {
    REF_getter<ContractHost> h = new ContractHost();
    h->granules = new TestGranuleIO();
    h->events = new TestEventSink();
    h->clock = new TestClock();
    h->source = new TestContractSourceIO();
    return h;
}
#ifdef KALL
static REF_getter<ContractHost> make_node() {
    REF_getter<ContractHost> h = new ContractHost();
    h->granules = new NodeGranuleIO();
    h->events = new NodeEventSink();
    h->clock = new NodeClock();
    h->source = new NodeContractSourceIO();
    return h;
}
#endif


struct TxCommand {
    std::string contract;
    std::string method;
    json params;
    bool expect_error = false;
    std::optional<json> expect_result;
    std::vector<jsiEvent> expect_events;
};

struct Transaction {
    jsiMsg msg;
    std::vector<TxCommand> commands;
};

struct TestCase {
    int64_t time = 0;
    json initial;
    std::vector<Transaction> transactions;
    std::vector<jsiEvent> expected_emits;
    json expected;
};

static bool split_granule_key(const std::string& key,
                              CONTRACT_id& c, CONTRACT_DATA_id& d,
                              std::string& combined)
{
    auto pos = key.find('/');
    if (pos == std::string::npos || pos == 0 || pos + 1 >= key.size())
        return false;
    c.container = key.substr(0, pos);
    d.container = key.substr(pos + 1);
    combined = c.container + d.container;
    return true;
}

static jsiMsg read_msg_json(const json& j) {
    jsiMsg m;
    if (!j.is_object()) return m;
    auto get_str = [&](const char* k) -> std::string {
        auto it = j.find(k);
        if (it == j.end() || it->is_null()) return {};
        return it->get<std::string>();
    };
    auto get_i64 = [&](const char* k) -> int64_t {
        auto it = j.find(k);
        if (it == j.end() || it->is_null()) return 0;
        return it->get<int64_t>();
    };
    m.sender         = get_str("sender");
    m.value          = get_i64("value");
    m.gasLimit       = get_i64("gasLimit");
    m.blockTimestamp = get_i64("blockTimestamp");
    m.blockHeight    = get_i64("blockHeight");
    m.nonce          = get_i64("nonce");
    m.chainId        = get_i64("chainId");
    m.txHash         = get_str("txHash");
    m.txIndex        = get_i64("txIndex");
    return m;
}

static void read_events_json(const json& arr, std::vector<jsiEvent>& out) {
    if (!arr.is_array()) return;
    for (auto& e : arr) {
        if (!e.is_object()) continue;
        jsiEvent ev;
        ev.contract = e.value("contract", std::string{});
        ev.name     = e.value("name", std::string{});
        auto it = e.find("value");
        if (it != e.end()) {
            ev.value = it->dump();
        }
        out.push_back(std::move(ev));
    }
}

static std::optional<json> parse_tx_js(const std::string& path) {
    JSIRuntime* rt = JSI_NewRuntime();
    if (!rt) return std::nullopt;
    JSIContext* ctx = JSI_NewContext(rt);
    if (!ctx) { JSI_FreeRuntime(rt); return std::nullopt; }

    std::string src;
    {
        FILE* f = fopen(path.c_str(), "rb");
        if (!f) { JSI_FreeContext(ctx); JSI_FreeRuntime(rt); return std::nullopt; }
        fseek(f, 0, SEEK_END); long size = ftell(f); fseek(f, 0, SEEK_SET);
        src.resize(size);
        auto sz = fread(&src[0], 1, size, f);
        fclose(f);
        if (sz != (size_t)size) { JSI_FreeContext(ctx); JSI_FreeRuntime(rt); return std::nullopt; }
    }

    JSIValue compiled = JSI_Eval(ctx, src.data(), src.size(), path.c_str(),
        JSI_EVAL_FLAG_STRICT | JSI_EVAL_TYPE_GLOBAL | JSI_EVAL_FLAG_COMPILE_ONLY);
    if (JSI_IsException(compiled)) {
        print_exception(ctx);
        JSI_FreeContext(ctx); JSI_FreeRuntime(rt);
        return std::nullopt;
    }

    JSIValue result = JSI_EvalFunction(ctx, compiled);
    if (JSI_IsException(result)) {
        print_exception(ctx);
        JSI_FreeValue(ctx, result);
        JSI_FreeContext(ctx); JSI_FreeRuntime(rt);
        return std::nullopt;
    }
    if (!JSI_IsObject(result)) {
        std::cerr << path << ": result is not an object" << std::endl;
        JSI_FreeValue(ctx, result);
        JSI_FreeContext(ctx); JSI_FreeRuntime(rt);
        return std::nullopt;
    }

    json j;
    try {
        j = json_from_jsi(ctx, result);
    } catch (const std::exception& e) {
        std::cerr << "exception: " << path << ": " << e.what() << std::endl;
        JSI_FreeValue(ctx, result);
        JSI_FreeContext(ctx);
        JSI_FreeRuntime(rt);
        return std::nullopt;
    }

    JSI_FreeValue(ctx, result);
    JSI_FreeContext(ctx);
    JSI_FreeRuntime(rt);
    return j;
}

bool load_test(const std::string& path, TestCase& out) {
    auto opt = parse_tx_js(path);
    if (!opt) return false;
    const json& j = *opt;

    if (!j.is_object()) {
        std::cerr << path << ": top-level not an object" << std::endl;
        return false;
    }

    out.time = j.value("time", (int64_t)0);

    auto init_it = j.find("initial");
    if (init_it != j.end() && init_it->is_object()) {
        out.initial = *init_it;
    } else {
        out.initial = json::object();
    }

    auto exp_it = j.find("expected");
    if (exp_it != j.end() && exp_it->is_object()) {
        out.expected = *exp_it;
    } else {
        out.expected = json::object();
    }

    auto txs_it = j.find("transactions");
    if (txs_it != j.end() && txs_it->is_array()) {
        for (auto& item : *txs_it) {
            if (!item.is_object()) continue;
            Transaction tx;
            auto mv = item.find("msg");
            if (mv != item.end() && mv->is_object()) tx.msg = read_msg_json(*mv);

            auto cmds = item.find("commands");
            if (cmds != item.end() && cmds->is_array()) {
                for (auto& c : *cmds) {
                    if (!c.is_object()) continue;
                    TxCommand cmd;
                    cmd.contract = c.value("contract", std::string{});
                    cmd.method   = c.value("method", std::string{});

                    auto pv = c.find("params");
                    cmd.params = (pv != c.end()) ? *pv : json::object();

                    auto ev = c.find("expect_error");
                    if (ev != c.end()) cmd.expect_error = ev->get<bool>();

                    auto rv = c.find("expect_result");
                    if (rv != c.end() && !rv->is_null()) cmd.expect_result = *rv;

                    auto ee = c.find("expect_events");
                    if (ee != c.end()) read_events_json(*ee, cmd.expect_events);

                    tx.commands.push_back(std::move(cmd));
                }
            }
            out.transactions.push_back(std::move(tx));
        }
    }

    auto emits_it = j.find("expected_emits");
    if (emits_it != j.end()) read_events_json(*emits_it, out.expected_emits);

    return true;
}

static void apply_initial(const TestCase& tc, const REF_getter<ContractHost>& host) {
    auto* test_io = static_cast<TestGranuleIO*>(host->granules.get());
    auto* test_events = static_cast<TestEventSink*>(host->events.get());
    auto* test_clock = static_cast<TestClock*>(host->clock.get());

    test_io->data.clear();
    test_events->all_events.clear();
    test_events->current_events.clear();
    test_clock->set(tc.time);

    for (auto it = tc.initial.begin(); it != tc.initial.end(); ++it) {
        const std::string& key = it.key();
        const json& v = it.value();
        std::string s = v.dump();
        if (s.size() > MAX_GRANULE_SIZE) {
            std::cerr << "initial[" << key << "]: too large" << std::endl;
            continue;
        }
        CONTRACT_id c; CONTRACT_DATA_id d; std::string combined;
        if (!split_granule_key(key, c, d, combined)) {
            std::cerr << "initial[" << key << "]: bad granule key (expected contract/data)" << std::endl;
            continue;
        }
        test_io->data[combined] = Granule{ std::move(s), 0, 0 };
    }
}

static bool json_equal(const json& a, const json& b) {
    return a == b;
}

static bool run_transactions(JSIRuntime* rt, JSIContext* runner_ctx, TestCase& tc,
                             const REF_getter<ContractHost>& host)
{
    auto* test_events = static_cast<TestEventSink*>(host->events.get());
    auto* test_io = static_cast<TestGranuleIO*>(host->granules.get());

    for (auto& tx : tc.transactions) {
        auto saved = test_io->data;
        auto saved_events = test_events->all_events;
        bool tx_aborted = false;

        for (auto& cmd : tx.commands) {
            CONTRACT_id cid;
            cid.container=cmd.contract;
            auto cc = host->get_contract(cid);
            if (!cc.valid()) { std::cerr << "contract not loaded: " << cmd.contract << std::endl; return false; }
            auto mit = cc->methods.find(cmd.method);
            if (mit == cc->methods.end()) {
                std::cerr << "method not found: " << cmd.contract << "." << cmd.method << std::endl;
                return false;
            }

            test_events->current_events.clear();

            CallContext call_ctx;
            call_ctx.contract = cc;
            call_ctx.in_view = false;
            call_ctx.msg = tx.msg;
            call_ctx.depth = 0;
            call_ctx.call_stack = new CallStack;
            call_ctx.host = host;

            void* saved_opaque = JSI_GetContextOpaque(cc->ctx);
            JSI_SetContextOpaque(cc->ctx, &call_ctx);

            JSIValue result = cc->call_contract_method(cmd.method, cmd.params, tx.msg, false,
                                                   call_ctx.call_stack, host);
            JSI_SetContextOpaque(cc->ctx, saved_opaque);

            for (auto& ev : call_ctx.emits) test_events->current_events.push_back(ev);

            if (JSI_IsException(result)) {
                JSI_FreeValue(cc->ctx, result);
                test_io->data = std::move(saved);
                test_events->all_events = std::move(saved_events);
                test_events->current_events.clear();
                if (!cmd.expect_error) {
                    std::cerr << "unexpected exception in " << cmd.contract << "." << cmd.method << std::endl;
                    return false;
                }
                tx_aborted = true;
                break;
            }
            if (cmd.expect_error) {
                std::cerr << "expected error, but got result in "
                          << cmd.contract << "." << cmd.method << std::endl;
                JSI_FreeValue(cc->ctx, result);
                test_io->data = std::move(saved);
                test_events->all_events = std::move(saved_events);
                return false;
            }

            if (cmd.expect_result) {
                std::string act_str = serialize_value(cc->ctx, result);
                json act;
                try {
                    act = json::parse(act_str);
                } catch (const std::exception& e) {
                    std::cerr << "result in " << cmd.contract << "." << cmd.method
                              << " is not JSON: " << act_str << " (" << e.what() << ")" << std::endl;
                    JSI_FreeValue(cc->ctx, result);
                    test_io->data = std::move(saved);
                    test_events->all_events = std::move(saved_events);
                    return false;
                }
                if (!json_equal(*cmd.expect_result, act)) {
                    std::cerr << "result mismatch in " << cmd.contract << "." << cmd.method
                              << ": expected " << cmd.expect_result->dump()
                              << ", got " << act.dump() << std::endl;
                    JSI_FreeValue(cc->ctx, result);
                    test_io->data = std::move(saved);
                    test_events->all_events = std::move(saved_events);
                    return false;
                }
            }

            if (!cmd.expect_events.empty()) {
                if (test_events->current_events.size() != cmd.expect_events.size()) {
                    std::cerr << "  events count mismatch in " << cmd.contract << "." << cmd.method
                              << ": expected " << cmd.expect_events.size()
                              << ", got " << test_events->current_events.size() << std::endl;
                    JSI_FreeValue(cc->ctx, result);
                    test_io->data = std::move(saved);
                    test_events->all_events = std::move(saved_events);
                    return false;
                }
                for (size_t k = 0; k < cmd.expect_events.size(); ++k) {
                    if (cmd.expect_events[k].contract != test_events->current_events[k].contract ||
                        cmd.expect_events[k].name     != test_events->current_events[k].name) {
                        std::cerr << "  event[" << k << "] mismatch (contract/name) in "
                                  << cmd.contract << "." << cmd.method << std::endl;
                        JSI_FreeValue(cc->ctx, result);
                        test_io->data = std::move(saved);
                        test_events->all_events = std::move(saved_events);
                        return false;
                    }
                    json exp_v, act_v;
                    try {
                        exp_v = json::parse(cmd.expect_events[k].value);
                        act_v = json::parse(test_events->current_events[k].value);
                    } catch (const std::exception& e) {
                        std::cerr << "  event[" << k << "] not JSON: " << e.what() << std::endl;
                        JSI_FreeValue(cc->ctx, result);
                        test_io->data = std::move(saved);
                        test_events->all_events = std::move(saved_events);
                        return false;
                    }
                    if (!json_equal(exp_v, act_v)) {
                        std::cerr << "  event[" << k << "] mismatch in "
                                  << cmd.contract << "." << cmd.method << std::endl;
                        JSI_FreeValue(cc->ctx, result);
                        test_io->data = std::move(saved);
                        test_events->all_events = std::move(saved_events);
                        return false;
                    }
                }
            }

            for (auto& ev : test_events->current_events) test_events->all_events.push_back(ev);
            JSI_FreeValue(cc->ctx, result);
        }

        if (tx_aborted) continue;
    }
    return true;
}

static bool compare_emits(const std::vector<jsiEvent>& actual,
                          const std::vector<jsiEvent>& expected)
{
    if (actual.size() != expected.size()) {
        std::cerr << "  emits count mismatch: expected " << expected.size()
                  << ", got " << actual.size() << std::endl;
        for (auto& e : expected)
            std::cerr << "    expected: " << e.contract << "." << e.name
                      << "(" << e.value << ")" << std::endl;
        for (auto& e : actual)
            std::cerr << "    actual:   " << e.contract << "." << e.name
                      << "(" << e.value << ")" << std::endl;
        return false;
    }
    for (size_t i = 0; i < actual.size(); ++i) {
        if (actual[i].contract != expected[i].contract ||
            actual[i].name     != expected[i].name) {
            std::cerr << "  emits[" << i << "] mismatch (contract/name)" << std::endl;
            return false;
        }
        json a, b;
        try {
            a = json::parse(actual[i].value);
            b = json::parse(expected[i].value);
        } catch (const std::exception& e) {
            std::cerr << "  emits[" << i << "] not JSON: " << e.what() << std::endl;
            return false;
        }
        if (!json_equal(a, b)) {
            std::cerr << "  emits[" << i << "] mismatch: expected "
                      << b.dump() << ", got " << a.dump() << std::endl;
            return false;
        }
    }
    return true;
}

static bool compare_expected(JSIContext*, const TestCase& tc,
                             const REF_getter<ContractHost>& host) {
    auto* test_io = static_cast<TestGranuleIO*>(host->granules.get());
    bool ok = true;
    for (auto it = tc.expected.begin(); it != tc.expected.end(); ++it) {
        const std::string& key = it.key();
        const json& exp_j = it.value();

        CONTRACT_id c; CONTRACT_DATA_id d; std::string combined;
        if (!split_granule_key(key, c, d, combined)) {
            std::cerr << "  expected[" << key << "]: bad granule key" << std::endl;
            ok = false;
            continue;
        }

        auto it_g = test_io->data.find(combined);
        if (it_g == test_io->data.end()) {
            std::cerr << "  missing granule: " << key << std::endl;
            ok = false;
            continue;
        }
        json act_j;
        try {
            act_j = json::parse(it_g->second.value);
        } catch (const std::exception& e) {
            std::cerr << "  granule " << key << " not JSON: " << e.what() << std::endl;
            ok = false;
            continue;
        }
        if (!json_equal(exp_j, act_j)) {
            std::cerr << "  mismatch at " << key
                      << ": expected " << exp_j.dump()
                      << ", got " << act_j.dump() << std::endl;
            ok = false;
        }
    }
    return ok;
}

std::vector<std::string> list_directory(const std::string& path) {
    std::vector<std::string> result;
    DIR* dir = opendir(path.c_str());
    if (!dir) return result;
    struct dirent* e;
    while ((e = readdir(dir)) != nullptr) {
        if (!std::strcmp(e->d_name, ".") || !std::strcmp(e->d_name, "..")) continue;
        std::string full = path + "/" + e->d_name;
        struct stat st;
        if (stat(full.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) continue;
        result.emplace_back(e->d_name);
    }
    closedir(dir);
    std::sort(result.begin(), result.end());
    return result;
}

int main() {
    JSIRuntime* rt = JSI_NewRuntime();
    if (!rt) return 1;
    JSIContext* ctx = JSI_NewContext(rt);
    if (!ctx) { JSI_FreeRuntime(rt); return 1; }

    auto dir = list_directory("tx");
    int failures = 0, total = 0;

    for (auto& file : dir) {
        if (file.size() < 6 || file.substr(file.size() - 6) != ".tx.js") continue;
        total++;
        std::string path = "tx/" + file;
        std::cout << "Running: " << path << std::endl;

        TestCase tc;
        if (!load_test(path, tc)) { failures++; continue; }

        REF_getter<ContractHost> host = make_test();
        apply_initial(tc, host);

        if (!run_transactions(rt, ctx, tc, host)) { failures++; continue; }
        auto* test_events = static_cast<TestEventSink*>(host->events.get());
        if (!compare_emits(test_events->all_events, tc.expected_emits)) { failures++; continue; }
        if (!compare_expected(ctx, tc, host)) { failures++; continue; }

        std::cout << "  PASS" << std::endl;
    }

    JSI_FreeContext(ctx);
    JSI_FreeRuntime(rt);

    std::cout << total - failures << "/" << total << " passed" << std::endl;
    return failures ? 1 : 0;
}
