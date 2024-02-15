#pragma once
#include "defs.h"
#include "nnue.h"
#include <vector>

struct ThreadInfo {
  uint64_t zobrist_key; // hash key of the position we're currently on
  uint16_t thread_id;   // ID of the thread
  GameHistory game_hist[GameSize]; // all positions from earlier in the game
  uint16_t game_ply;               // how far we're into the game
  uint16_t search_ply;             // depth that we are in the search tree
  uint64_t nodes;                  // Total nodes searched so far this search
  std::chrono::steady_clock::time_point
      start_time; // Start time of the search

  uint32_t max_time;
  uint32_t opt_time;
  uint16_t time_checks;
  bool stop;
  NNUE_State nnue_state;

  uint8_t max_iter_depth = MaxSearchDepth;
};

uint64_t TT_size = (1 << 20);
uint64_t TT_mask = TT_size - 1;
std::vector<TTEntry> TT(TT_size);

void clear_TT() {
  memset(&TT[0], 0, TT_size * sizeof(TT[0]));
} // Clears the TT of all data.

void resize_TT(int size){
  int target_size = size * 1024 * 1024 / sizeof(TTEntry);
  int tt_size = 1024; 
  while (tt_size * 2 <= target_size){
    tt_size *= 2;
  }
  TT_size = tt_size;
  TT.reserve(TT_size);
  TT_mask = tt_size - 1;
  clear_TT();
}

void insert_entry(
    uint64_t hash, int depth, Move best_move, int32_t score,
    uint8_t bound_type) { // Inserts an entry into the transposition table.
  int indx = hash & TT_mask;
  TT[indx].position_key = static_cast<uint32_t>(get_hash_upper_bits(hash)),
  TT[indx].depth = static_cast<uint8_t>(depth), TT[indx].type = bound_type,
  TT[indx].score = score;
  TT[indx].best_move = best_move;
}

uint64_t calculate(
    Position position) { // Calculates the zobrist key of a given position.
                         // Useful when initializing positions, in search though
                         // incremental updates are faster.
  uint64_t hash = 0;
  for (int indx : StandardToMailbox) {
    int piece = position.board[indx];
    if (piece) {
      hash ^= zobrist_keys[get_zobrist_key(piece, standard(indx))];
    }
  }
  if (position.color) {
    hash ^= zobrist_keys[side_index];
  }
  if (position.ep_square != 255) {
    hash ^= zobrist_keys[ep_index];
  }
  for (int indx = castling_index; indx < 778; indx++) {
    if (position.castling_rights[indx > 775][(indx & 1)]) {
      hash ^= zobrist_keys[indx];
    }
  }
  return hash;
}

int64_t time_elapsed(std::chrono::steady_clock::time_point start_time) {
  auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
                            now - start_time)
                            .count();
}