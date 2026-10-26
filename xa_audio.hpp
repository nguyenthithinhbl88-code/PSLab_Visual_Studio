#pragma once
#include <array>
#include <cstdint>
#include <deque>
namespace ps1 {
struct XaAudio {
 std::array<int,2> previous{},older{};
 std::deque<std::array<int16_t,2>> pcm;
 uint64_t sectors=0,frames=0,dropped=0,invalid=0;
 bool decode(const std::array<uint8_t,2352>& raw);
};
}
