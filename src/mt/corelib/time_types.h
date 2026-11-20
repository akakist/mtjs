#pragma once
#include <cstdint>
#include <string>

#include "ioBuffer.h"

/// Сильные типы времени и длительностей.
/// Момент и длительность — разные сущности: время + время бессмысленно,
/// время + длительность = время. Компилятор ловит смешение единиц.
///
/// Провод не меняется: на диске/в сети это те же int64_t (поле v).

/// Абсолютный момент: микросекунды от эпохи (block_timestamp, _update_time).
struct TIME_mks
{
    int64_t v = 0;
    TIME_mks() = default;
    explicit TIME_mks(int64_t x): v(x) {}

    bool operator==(const TIME_mks& o) const { return v == o.v; }
    bool operator!=(const TIME_mks& o) const { return v != o.v; }
    bool operator< (const TIME_mks& o) const { return v <  o.v; }
    bool operator<=(const TIME_mks& o) const { return v <= o.v; }
    bool operator> (const TIME_mks& o) const { return v >  o.v; }
    bool operator>=(const TIME_mks& o) const { return v >= o.v; }

    std::string str() const { return std::to_string(v); }
};

/// Абсолютный момент в секундах (genesis_time, wall-clock, документация).
struct TIME_sec
{
    int64_t v = 0;
    TIME_sec() = default;
    explicit TIME_sec(int64_t x): v(x) {}

    bool operator==(const TIME_sec& o) const { return v == o.v; }
    bool operator!=(const TIME_sec& o) const { return v != o.v; }
    bool operator< (const TIME_sec& o) const { return v <  o.v; }

    std::string str() const { return std::to_string(v); }
};

/// Длительность в микросекундах (внутреннее хранение TTL).
struct TTL_mks
{
    int64_t v = 0;
    TTL_mks() = default;
    explicit TTL_mks(int64_t x): v(x) {}

    bool operator==(const TTL_mks& o) const { return v == o.v; }
    bool operator!=(const TTL_mks& o) const { return v != o.v; }
    bool operator< (const TTL_mks& o) const { return v <  o.v; }
    bool operator> (const TTL_mks& o) const { return v >  o.v; }
    bool operator<=(const TTL_mks& o) const { return v <= o.v; }
    bool operator>=(const TTL_mks& o) const { return v >= o.v; }

    bool positive() const { return v > 0; }
};

/// Длительность в секундах (то, что задаёт пользователь: JS, контракты, доки).
struct TTL_sec
{
    int64_t v = 0;
    TTL_sec() = default;
    explicit TTL_sec(int64_t x): v(x) {}

    bool operator==(const TTL_sec& o) const { return v == o.v; }
    bool operator!=(const TTL_sec& o) const { return v != o.v; }
    bool operator< (const TTL_sec& o) const { return v <  o.v; }
    bool operator> (const TTL_sec& o) const { return v >  o.v; }
    bool operator<=(const TTL_sec& o) const { return v <= o.v; }
    bool operator>=(const TTL_sec& o) const { return v >= o.v; }

    bool positive() const { return v > 0; }
};

/// Единственные точки преобразования единиц.
inline TTL_mks to_mks(TTL_sec d) { return TTL_mks(d.v * 1000000); }
inline TTL_sec to_sec(TTL_mks d) { return TTL_sec(d.v / 1000000); }

inline TIME_mks to_mks(TIME_sec d) { return TIME_mks(d.v * 1000000); }
inline TIME_sec to_sec(TIME_mks d) { return TIME_sec(d.v / 1000000); }

/// Время + длительность = время (единственная разрешённая арифметика).
inline TIME_mks operator+(TIME_mks t, TTL_mks d) { return TIME_mks(t.v + d.v); }
inline TIME_mks operator-(TIME_mks t, TTL_mks d) { return TIME_mks(t.v - d.v); }

/// Разность моментов — это длительность.
inline TTL_mks operator-(TIME_mks a, TIME_mks b) { return TTL_mks(a.v - b.v); }

/// Совместимость с упаковкой/логами там, где типы ещё не протянуты.
inline int64_t raw(TIME_mks t) { return t.v; }
inline int64_t raw(TTL_mks t) { return t.v; }
inline int64_t raw(TTL_sec t) { return t.v; }

// ── упаковка: на проводе те же int64_t, меняется только тип в коде ──
inline outBuffer& operator<<(outBuffer& o, TIME_mks  t) { o << t.v; return o; }
inline outBuffer& operator<<(outBuffer& o, TIME_sec  t) { o << t.v; return o; }
inline outBuffer& operator<<(outBuffer& o, TTL_mks   t) { o << t.v; return o; }
inline outBuffer& operator<<(outBuffer& o, TTL_sec   t) { o << t.v; return o; }

inline inBuffer&  operator>>(inBuffer& i, TIME_mks& t) { i >> t.v; return i; }
inline inBuffer&  operator>>(inBuffer& i, TIME_sec& t) { i >> t.v; return i; }
inline inBuffer&  operator>>(inBuffer& i, TTL_mks&  t) { i >> t.v; return i; }
inline inBuffer&  operator>>(inBuffer& i, TTL_sec&  t) { i >> t.v; return i; }
