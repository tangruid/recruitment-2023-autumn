#include "graph.hh"
#include <thread>
#include <vector>
#include <mutex>
#include <iostream>
#include <immintrin.h>
void Floyd(Graph &target, int start, int end, int k)
{
    int *Distance = target.get_raw_ptr();

    int ij = 0, ik = 0, kj = 0;
    int vertex_num_ = target.vertex_num();

    ij = start * vertex_num_;
    ik = start * vertex_num_ + k;
    int new_dist;
    for (int i = start; i < end; ++i)
    {
        kj = k * vertex_num_;
        int Distance_ik = Distance[ik];
        int j = 0;
        for (; j < vertex_num_ - 8; j += 8)
        {
            __m256i Distance_kj = _mm256_load_si256((__m256i *)(Distance + kj));
            __m256i Dis = _mm256_set1_epi32(Distance_ik);
            Dis = _mm256_add_epi32(Dis, Distance_kj);
            __m256i Distance_ij = _mm256_load_si256((__m256i *)(Distance + ij));
            Dis = _mm256_min_epi32(Distance_ij, Dis);
            _mm256_storeu_si256((__m256i *)(Distance + ij), Dis);
            ij += 8;
            kj += 8;
        }
        for (; j < vertex_num_; ++j)
        {

            new_dist = Distance_ik + Distance[kj];
            if (new_dist < Distance[ij])
            {
                target(i, j) = new_dist;
            }
            ij++;
            kj++;
        }
        ik += vertex_num_;
    }
}

Graph Graph::apsp()
{
    Graph result(*this);
    std::vector<std::thread> threads;

    // const int Num_threads = 110;//调整线程数量，自动分配各线程数据量
    // int thread_len = vertex_num_ / Num_threads;
    const int thread_len = 64; // 提前分配数据量，自动分配线程数量
    int Num_threads = vertex_num_ % thread_len == 0 ? vertex_num_ / thread_len : vertex_num_ / thread_len + 1;

    for (int k = 0; k < vertex_num_; k++)
    {
        for (int thread = 0; thread < Num_threads; ++thread)
        {
            int start = thread * thread_len;
            int end = (thread == Num_threads - 1) ? vertex_num_ : (thread + 1) * thread_len;
            threads.emplace_back(Floyd, std::ref(result), start, end, k);
        }
        for (auto &t : threads)
        {
            t.join();
        }

        // 清空线程容器，准备下一轮
        threads.clear();
    }
    return result;
}

// opt_example
/*
Graph Graph::apsp()
{
    Graph result(*this);
    int *Distance = result.get_raw_ptr();
    int ij = 0, ik = 0, kj = 0;
    for (int k = 0; k < vertex_num_; ++k)
    {
        ij = 0;
        ik = k;
        for (int i = 0; i < vertex_num_; ++i)
        {
            kj = k * vertex_num_;
            int Distance_ik = Distance[ik];
            for (int j = 0; j < vertex_num_; ++j)
            {
                result(i, j) = std::min(Distance[ij], Distance_ik + Distance[kj]);
                ++ij;
                ++kj;
            }
            ik += vertex_num_;
        }
    }
    return result;
}
*/

// openMP + O2 优化
/*
//  Graph Graph::apsp()
//  {
//      Graph result(*this);

// #pragma omp parallel for
//     for (int k = 0; k < vertex_num_; ++k)
//     {
// #pragma omp parallel for
//         for (int i = 0; i < vertex_num_; ++i)
//         {
//             for (int j = 0; j < vertex_num_; ++j)
//             {
//                 result(i, j) = std::min(result(i, j), result(i, k) + result(k, j));
//             }
//         }
//     }

//     return result;
// }

*/

// First
/*
// Graph Graph::apsp()
// {
//     Graph result(*this);
//     for (int k = 0; k < vertex_num_; ++k)
//     {
//         for (int i = 0; i < vertex_num_; ++i)
//         {
//             int result_ik = result(i, k);

//             for (int j = 0; j < vertex_num_; ++j)
//             {
//                 // Minimize the number of function calls
//                 int new_dist = result_ik + result(k, j);
//                 // int &target = result(i, j);
//                 // if (new_dist < target)
//                 //     target = new_dist;
//                 if (new_dist < result(i, j))
//                 {
//                     result(i, j) = new_dist;
//                 }
//                 // result(i, j) = std::min(result(i, j), result_ik + result(k, j));
//             }
//         }
//     }

//     return result;
// }

*/

// 消除重复运算和不必要的函数调用（最终版）
// Graph Graph::apsp()
// {
//     Graph result(*this);
//     int *Distance = result.get_raw_ptr();
//     int ij = 0, ik = 0, kj = 0;
//     for (int k = 0; k < vertex_num_; ++k)
//     {
//         ik = k;
//         ij = 0;
//         for (int i = 0; i < vertex_num_; ++i)
//         {
//             kj = k * vertex_num_;
//             int Distance_ik = Distance[ik];
//             for (int j = 0; j < vertex_num_; ++j)
//             {
//                 int new_dist = Distance_ik + Distance[kj];
//                 if (new_dist < Distance[ij])
//                 {
//                     result(i, j) = new_dist;
//                 }
//                 ij++;
//                 kj++;
//             }
//             ik += vertex_num_;
//         }
//     }
//     return result;
// }

// unsuccessful version(pthread)
//  struct node
//  {
//      Graph &result;
//      int start;
//      int end;
//      node(Graph &temp, int x, int y) : result(temp), start(x), end(y) {}
//  };
//  pthread_mutex_t mutex;
//  void *Floyd(void *rank)
//  {
//      node *temp = (node *)rank;
//      Graph &target(temp->result);
//      int start = temp->start;
//      int end = temp->end;
//      int *Distance = target.get_raw_ptr();
//      int ij = 0, ik = 0, kj = 0;
//      int vertex_num_ = target.vertex_num();
//      for (int k = 0; k < vertex_num_; ++k)
//      {
//          ik = k;
//          ij = 0;
//          for (int i = start; i < end; ++i)
//          {
//              kj = k * vertex_num_;
//              int Distance_ik = Distance[ik];
//              for (int j = 0; j < vertex_num_; ++j)
//              {
//                  int new_dist = Distance_ik + Distance[kj];
//                  if (new_dist < Distance[ij])
//                  {
//                      pthread_mutex_lock(&mutex);
//                      target(i, j) = new_dist;
//                      pthread_mutex_unlock(&mutex);
//                  }
//                  ij++;
//                  kj++;
//              }
//              ik += vertex_num_;
//          }
//      }
//  }
//  Graph Graph::apsp()
//  {
//      Graph result(*this);
//      int *Distance = result.get_raw_ptr();
//      const int Num_threads = 2;
//      long thread;
//      pthread_t *thread_handles;
//      thread_handles = (pthread_t *)malloc(Num_threads * sizeof(pthread_t));
//      pthread_mutex_t mutex;
//      for (thread = 0; thread < Num_threads; thread++)
//      {
//          int start = thread * vertex_num_ / Num_threads;
//          int end = (thread + 1) * vertex_num_ / Num_threads;
//          node temp(result, start, end);
//          pthread_create(&thread_handles[thread], NULL, Floyd, (void *)&temp);
//      }
//      for (thread = 0; thread < Num_threads; thread++)
//      {
//          pthread_join(thread_handles[thread], NULL);
//      }
//      free(thread_handles);
//      free(&mutex);
//      return result;
//  }

// final (thread+函数简化)

// 数据分块处理
/*
// #include <thread>
// #include <vector>
// #include <mutex>
// #include <iostream>

// // std::mutex mtx;

// void Floyd(Graph &target, int start_x, int end_x, int start_y, int end_y, int k)
// {
//     int *Distance = target.get_raw_ptr();

//     int ij = 0, ik = 0, kj = 0;
//     int vertex_num_ = target.vertex_num();

//     // ij = start_x * vertex_num_ + start_y;
//     ik = start_x * vertex_num_ + k;
//     int new_dist;
//     for (int i = start_x; i < end_x; ++i)
//     {
//         ij = i * vertex_num_ + start_y;
//         kj = k * vertex_num_ + start_y;
//         int Distance_ik = Distance[ik];
//         for (int j = start_y; j < end_y; ++j)
//         {

//             new_dist = Distance_ik + Distance[kj];

//             // Ensure that target(i, j) is thread-safe before removing the lock
//             if (new_dist < Distance[ij])
//             {
//                 // std::lock_guard<std::mutex> lock(mtx);
//                 target(i, j) = new_dist;
//             }
//             ij++;
//             kj++;
//         }
//         ik += vertex_num_;
//     }
// }

// Graph Graph::apsp()
// {
//     Graph result(*this);
//     int *Distance = result.get_raw_ptr();
//     const int Num_threads = 8;
//     const int Num_threads2 = 8;

//     std::vector<std::thread> threads;
//     int thread_len = vertex_num_ / Num_threads;
//     int thread_len2 = vertex_num_ / Num_threads2;

//     for (int k = 0; k < vertex_num_; k++)
//     {
//         for (int thread = 0; thread < Num_threads; ++thread)
//         {
//             int start_x = thread * thread_len;
//             int end_x = (thread == Num_threads - 1) ? vertex_num_ : (thread + 1) * thread_len;
//             for (int thread_j = 0; thread_j < Num_threads2; ++thread_j)
//             {
//                 int start_y = thread_j * thread_len2;
//                 int end_y = (thread_j == Num_threads2 - 1) ? vertex_num_ : (thread_j + 1) * thread_len2;
//                 threads.emplace_back(Floyd, std::ref(result), start_x, end_x, start_y, end_y, k);
//             }
//         }
//         for (auto &t : threads)
//         {
//             t.join();
//         }

//         // 清空线程容器，准备下一轮
//         threads.clear();
//     }

//     return result;
// }

*/
