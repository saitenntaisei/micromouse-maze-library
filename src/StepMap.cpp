/**
 * @file StepMap.cpp
 * @brief マイクロマウスの迷路のステップマップを扱うクラス
 * @author Ryotaro Onuki <kerikun11+github@gmail.com>
 * @date 2017-11-05
 * @copyright Copyright 2017 Ryotaro Onuki <kerikun11+github@gmail.com>
 */
#include "../include/MazeLib/StepMap.h"

#include <algorithm>  //< for std::sort
#include <cmath>      //< for std::sqrt
#include <iomanip>    //< for std::setw
#include <queue>

namespace MazeLib {

StepMap::StepMap() {
  calcStraightCostTable();
  reset();
}
void StepMap::print(const Maze& maze, const Position p, const Direction d,
                    std::ostream& os) const {
  return print(maze, {d}, p.next(d + Direction::Back), os);
}
void StepMap::print(const Maze& maze, const Directions& dirs,
                    const Position start, std::ostream& os) const {
  /* preparation */
  std::vector<Pose> path;
  path.reserve(dirs.size());
  Position p = start;
  for (const auto d : dirs) path.push_back({p, d}), p = p.next(d);
  const int mazeSize = MAZE_SIZE;
  step_t maxStep = 0;
  for (const auto step : stepMap)
    if (step != STEP_MAX) maxStep = std::max(maxStep, step);
  const bool simple = (maxStep < 999);
  const step_t scaler =
      stepTable[stepTableSize - 1] - stepTable[stepTableSize - 2];
  const auto find = [&](const WallIndex& i) {
    return std::find_if(path.cbegin(), path.cend(), [&](const Pose& pose) {
      return WallIndex(pose.p, pose.d) == i;
    });
  };
  /* start to draw maze */
  for (int8_t y = mazeSize; y >= 0; --y) {
    /* Vertical Wall Line */
    if (y != mazeSize) {
      for (uint8_t x = 0; x <= mazeSize; ++x) {
        /* Vertical Wall */
        const auto w = maze.isWall(x, y, Direction::West);
        const auto k = maze.isKnown(x, y, Direction::West);
        const auto it = find(WallIndex(Position(x, y), Direction::West));
        if (it != path.cend())
          os << C_YE "\e[1m" << it->d << C_NO;
        else
          os << (k ? (w ? "|" : " ") : (C_RE "." C_NO));
        /* Cell */
        if (x != mazeSize) {
          step_t step = getStep(x, y);
          step = std::min(999, simple ? step : step / scaler);
          os << (step == 0 ? C_YE : C_BL) << std::setw(3) << step << C_NO;
        }
      }
      os << "\e[0K" << std::endl;  // clear from cursor position to end of line
    }
    /* Horizontal Wall Line */
    for (uint8_t x = 0; x < mazeSize; ++x) {
      /* Pillar */
      os << '+';
      /* Horizontal Wall */
      const auto w = maze.isWall(x, y, Direction::South);
      const auto k = maze.isKnown(x, y, Direction::South);
      const auto it = find(WallIndex(Position(x, y), Direction::South));
      if (it != path.cend())
        os << C_YE "\e[1m " << it->d << " " C_NO;
      else
        os << (k ? (w ? "---" : "   ") : (C_RE " . " C_NO));
    }
    os << '+' << "\e[0K" << std::endl;
  }
}
void StepMap::printFull(const Maze& maze, const Position p, const Direction d,
                        std::ostream& os) const {
  return printFull(maze, {d}, p.next(d + Direction::Back), os);
}
void StepMap::printFull(const Maze& maze, const Directions& dirs,
                        const Position start, std::ostream& os) const {
  /* preparation */
  std::vector<Pose> path;
  path.reserve(dirs.size());
  Position p = start;
  for (const auto d : dirs) path.push_back({p, d}), p = p.next(d);
  const int mazeSize = MAZE_SIZE;
  const auto find = [&](const WallIndex& i) {
    return std::find_if(path.cbegin(), path.cend(), [&](const Pose& pose) {
      return WallIndex(pose.p, pose.d) == i;
    });
  };
  /* start to draw maze */
  for (int8_t y = mazeSize; y >= 0; --y) {
    /* Vertical Wall Line */
    if (y != mazeSize) {
      for (uint8_t x = 0; x <= mazeSize; ++x) {
        /* Vertical Wall */
        const auto w = maze.isWall(x, y, Direction::West);
        const auto k = maze.isKnown(x, y, Direction::West);
        const auto it = find(WallIndex(Position(x, y), Direction::West));
        if (it != path.cend())
          os << C_YE "\e[1m" << it->d << C_NO;
        else
          os << (k ? (w ? "|" : " ") : (C_RE "." C_NO));
        /* Cell */
        if (x != mazeSize) {
          auto step = std::min((step_t)99999, getStep(x, y));
          os << (step == 0 ? C_YE : C_BL) << std::setw(5) << step << C_NO;
        }
      }
      os << std::endl;
    }
    /* Horizontal Wall Line */
    for (uint8_t x = 0; x < mazeSize; ++x) {
      /* Pillar */
      os << '+';
      /* Horizontal Wall */
      const auto w = maze.isWall(x, y, Direction::South);
      const auto k = maze.isKnown(x, y, Direction::South);
      const auto it = find(WallIndex(Position(x, y), Direction::South));
      if (it != path.cend())
        os << C_YE "\e[1m  " << it->d << "  " C_NO;
      else
        os << (k ? (w ? "-----" : "     ") : (C_RE "  .  " C_NO));
    }
    os << '+' << std::endl;
  }
}
void StepMap::update(const Maze& maze, const Positions& dest,
                     const bool knownOnly, const bool simple) {
  MAZE_DEBUG_PROFILING_START(0)
  /* 計算を高速化するため、迷路の大きさを制限 */
  int8_t min_x = maze.getMinX();
  int8_t max_x = maze.getMaxX();
  int8_t min_y = maze.getMinY();
  int8_t max_y = maze.getMaxY();
  for (const auto p : dest) {  //< ゴールを含めないと導出不可能になる
    min_x = std::min(p.x, min_x);
    max_x = std::max(p.x, max_x);
    min_y = std::min(p.y, min_y);
    max_y = std::max(p.y, max_y);
  }
  min_x -= 1, min_y -= 1, max_x += 2, max_y += 2;  //< 外周を許す
  /* 全区画のステップを最大値に設定 */
  reset();
  /* ステップの更新予約のキュー */
#define STEP_MAP_USE_PRIORITY_QUEUE 1
#if STEP_MAP_USE_PRIORITY_QUEUE
  struct Element {
    Position p;
    step_t s;
    bool operator<(const Element& e) const { return s > e.s; }
  };
  std::priority_queue<Element> q;
#else
  std::queue<Position> q;
#endif
  /* destのステップを0とする */
  for (const auto p : dest)
    if (p.isInsideOfField())
#if STEP_MAP_USE_PRIORITY_QUEUE
      setStep(p, 0), q.push({p, 0});
#else
      setStep(p, 0), q.push(p);
#endif
  /* ステップの更新がなくなるまで更新処理 */
  while (!q.empty()) {
#if MAZE_DEBUG_PROFILING
    queueSizeMax = std::max(queueSizeMax, static_cast<int>(q.size()));
#endif
    /* 注目する区画を取得 */
#if STEP_MAP_USE_PRIORITY_QUEUE
    const auto focus = q.top().p;
    const auto focus_step_q = q.top().s;
#else
    const auto focus = q.front();
#endif
    q.pop();
    /* 計算を高速化するため展開範囲を制限 */
    if (focus.x > max_x || focus.y > max_y || focus.x < min_x ||
        focus.y < min_y)
      continue;
    const auto focus_step = stepMap[focus.getIndex()];
#if STEP_MAP_USE_PRIORITY_QUEUE
    /* 枝刈り */
    if (focus_step < focus_step_q) continue;
#endif
    /* 周辺を走査 */
    for (const auto d : Direction::Along4()) {
      /* 直線で行けるところまで更新する */
      auto next = focus;
      for (int8_t i = 1;; ++i) {
        /* 壁あり or 既知壁のみで未知壁 ならば次へ */
        const auto next_wi = WallIndex(next, d);
        if (maze.isWall(next_wi) || (knownOnly && !maze.isKnown(next_wi)))
          break;
        next = next.next(d);  //< 移動
        /* 直線加速を考慮したステップを算出 */
        const step_t next_step = focus_step + (simple ? i : stepTable[i]);
        const auto next_index = next.getIndex();
        if (stepMap[next_index] <= next_step) break;  //< 更新の必要がない
        stepMap[next_index] = next_step;              //< 更新
        /* 再帰的に更新するためにキューにプッシュ */
#if STEP_MAP_USE_PRIORITY_QUEUE
        q.push({next, next_step});
#else
        q.push(next);
#endif
      }
    }
  }
  MAZE_DEBUG_PROFILING_END(0)
}
Directions StepMap::calcShortestDirections(const Maze& maze,
                                           const Position start,
                                           const Positions& dest,
                                           const bool knownOnly,
                                           const bool simple) {
  /* ステップマップを更新 */
  update(maze, dest, knownOnly, simple);
  Pose end;
  const auto shortestDirections = getStepDownDirections(
      maze, {start, Direction::Max}, end, knownOnly, simple, false);
  /* ゴール判定 */
  return stepMap[end.p.getIndex()] == 0 ? shortestDirections : Directions{};
}
Pose StepMap::calcNextDirections(const Maze& maze, const Pose& start,
                                 Directions& nextDirectionsKnown,
                                 Directions& nextDirectionCandidates) const {
  Pose end;
  nextDirectionsKnown =
      getStepDownDirections(maze, start, end, false, false, true);
  nextDirectionCandidates = getNextDirectionCandidates(maze, end);
  return end;
}
Directions StepMap::getStepDownDirections(const Maze& maze, const Pose& start,
                                          Pose& end, const bool knownOnly,
                                          const bool simple,
                                          const bool breakUnknown) const {
#if 1
  /* 最短経路となるスタートからの方向列 */
  Directions shortestDirections;
  auto& focus = end;
  /* start から順にステップマップを下る */
  focus = start;
  /* 確認 */
  if (!start.p.isInsideOfField()) return {};
  /* 周辺の走査; 未知壁の有無と最小ステップの方向を求める */
  while (1) {
    const auto focus_step = stepMap[focus.p.getIndex()];
    /* 終了条件 */
    if (focus_step == 0) break;
    /* 周辺を走査 */
    auto min_p = focus.p;
    auto min_d = Direction::Max;
    for (const auto d : Direction::Along4()) {
      /* 直線で行けるところまで探す */
      auto next = focus.p;  //< 隣接
      for (int8_t i = 1;; ++i) {
        /* 壁あり or 既知壁のみで未知壁 ならば次へ */
        if (maze.isWall(next, d) || (knownOnly && !maze.isKnown(next, d)))
          break;
        next = next.next(d);  //< 移動
        /* 直線加速を考慮したステップを算出 */
        const step_t next_step = focus_step - (simple ? i : stepTable[i]);
        /* エッジコストと一致するか確認 */
        if (stepMap[next.getIndex()] == next_step) {
          min_p = next, min_d = d;
          goto loop_exit;
        }
      }
    }
  loop_exit:
    /* 現在地よりステップが大きかったらなんかおかしい */
    if (focus_step <= stepMap[min_p.getIndex()]) break;
    /* 移動分を結果に追加 */
    while (focus.p != min_p) {
      /* breakUnknown のとき、未知壁を含むならば既知区間は終了 */
      if (breakUnknown && maze.unknownCount(focus.p)) return shortestDirections;
      focus = focus.next(min_d);
      shortestDirections.push_back(min_d);
    }
  }
  return shortestDirections;
#else
  /* ステップマップから既知区間進行方向列を生成 */
  Directions shortestDirections;
  /* start から順にステップマップを下る */
  end = start;
  /* 確認 */
  if (!start.p.isInsideOfField()) return {};
  while (1) {
    /* 周辺の走査; 未知壁の有無と、最小ステップの方向を求める */
    auto min_pose = end;
    auto min_step = STEP_MAX;
    for (const auto d : Direction::Along4()) {
      auto next = end.p;  //< 隣接
      for (int8_t i = 1; i < MAZE_SIZE; ++i) {
        /* 壁あり or 既知壁のみで未知壁 ならば次へ */
        if (maze.isWall(next, d) || (knownOnly && !maze.isKnown(next, d)))
          break;
        next = next.next(d);  //< 隣接区画へ移動
        /* 現時点の min_step よりステップが小さければ更新 */
        const auto next_step = stepMap[next.getIndex()];
        if (min_step <= next_step) break;
        min_step = next_step;
        min_pose = Pose{next, d};
      }
    }
    /* 現在地よりステップが大きかったらなんかおかしい */
    if (stepMap[end.p.getIndex()] <= min_step) break;
    /* 移動分を結果に追加 */
    while (end.p != min_pose.p) {
      /* breakUnknown のとき、未知壁を含むならば既知区間は終了 */
      if (breakUnknown && maze.unknownCount(end.p)) return shortestDirections;
      end = end.next(min_pose.d);
      shortestDirections.push_back(min_pose.d);
    }
  }
  return shortestDirections;
#endif
}
Directions StepMap::getNextDirectionCandidates(const Maze& maze,
                                               const Pose& focus) const {
  /* 直線優先で進行方向の候補を抽出。全方位 STEP_MAX だと空になる */
  Directions dirs;
  dirs.reserve(4);
  for (const auto d : {focus.d + Direction::Front, focus.d + Direction::Left,
                       focus.d + Direction::Right, focus.d + Direction::Back})
    if (!maze.isWall(focus.p, d) && getStep(focus.p.next(d)) != STEP_MAX)
      dirs.push_back(d);
  /* コストの低い順に並べ替え */
  std::sort(dirs.begin(), dirs.end(),
            [&](const Direction d1, const Direction d2) {
              return getStep(focus.p.next(d1)) < getStep(focus.p.next(d2));
            });
#if 1
  /* 未知壁優先で並べ替え(未知壁同士ならばコストが低い順) */
  std::sort(dirs.begin(), dirs.end(),
            [&](const Direction d1, const Direction d2) {
              return (maze.unknownCount(focus.p.next(d1)) &&
                      !maze.unknownCount(focus.p.next(d2)));
            });
#endif
#if 1
  /* 直進優先に並べ替え */
  std::sort(dirs.begin(), dirs.end(),
            [&](const Direction d1, const Direction d2
                __attribute__((unused))) { return d1 == focus.d; });
#endif
  return dirs;
}
void StepMap::appendStraightDirections(const Maze& maze,
                                       Directions& shortestDirections,
                                       const bool knownOnly,
                                       const bool diagEnabled) {
  /* ゴール区画までたどる */
  auto p = maze.getStart();
  for (const auto d : shortestDirections) p = p.next(d);
  if (shortestDirections.size() < 2) return;
  auto prev_dir = shortestDirections[shortestDirections.size() - 1 - 1];
  auto dir = shortestDirections[shortestDirections.size() - 1];
  /* ゴール区画内を行けるところまで直進(斜め考慮)する */
  bool loop = true;
  while (loop) {
    loop = false;
    /* 斜めを考慮した進行方向を列挙する */
    Directions dirs;
    const auto rel_dir = Direction(dir - prev_dir);
    if (diagEnabled && rel_dir == Direction::Left)
      dirs = {Direction(dir + Direction::Right), dir};
    else if (diagEnabled && rel_dir == Direction::Right)
      dirs = {Direction(dir + Direction::Left), dir};
    else
      dirs = {dir};
    /* 候補のうち行ける方向に行く */
    for (const auto d : dirs) {
      if (!maze.isWall(p, d) && (!knownOnly || maze.isKnown(p, d))) {
        shortestDirections.push_back(d);
        p = p.next(d);
        prev_dir = dir;
        dir = d;
        loop = true;
        break;
      }
    }
  }
}
/**
 * @brief 台形加速を考慮したコストを生成する関数
 *
 * @param i マスの数
 * @param am 最大加速度
 * @param vs 始点速度
 * @param vm 飽和速度
 * @param seg 1マスの長さ
 * @return StepMap::step_t コスト
 */
static StepMap::step_t calcStraightCost(const int i, const float am,
                                        const float vs, const float vm,
                                        const float seg) {
  const auto d = seg * i;  //< i 区画分の走行距離
  /* グラフの面積から時間を求める */
  const auto d_thr = (vm * vm - vs * vs) / am;  //< 最大速度に達する距離
  if (d < d_thr)
    return 2 * (std::sqrt(vs * vs + am * d) - vs) / am * 1000;  //< 三角加速
  else
    return (am * d + (vm - vs) * (vm - vs)) / (am * vm) * 1000;  //< 台形加速
}
void StepMap::calcStraightCostTable() {
  const float vs = 420.0f;      //< 基本速度 [mm/s]
  const float am_a = 4200.0f;   //< 最大加速度 [mm/s/s]
  const float vm_a = 1500.0f;   //< 飽和速度 [mm/s]
  const float seg_a = 90.0f;    //< 区画の長さ [mm]
  const float t_turn = 287.0f;  //< 小回り90度ターンの時間 [ms]
  stepTable[0] = 0;             //< [0] は使用しない
  for (int i = 1; i < stepTableSize; ++i) {
    /* 1歩目は90度ターンとみなす */
    stepTable[i] = t_turn + calcStraightCost(i - 1, am_a, vs, vm_a, seg_a);
  }
  /* コストの合計が 65,535 [ms] を超えないようにスケーリング */
  for (int i = 0; i < stepTableSize; ++i) {
    stepTable[i] /= scalingFactor;
#if 0
    MAZE_LOGI << "stepTable[" << i << "]:\t" << stepTable[i] << std::endl;
#endif
  }
}

}  // namespace MazeLib
