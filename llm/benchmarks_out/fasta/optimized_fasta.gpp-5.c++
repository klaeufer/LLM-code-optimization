#include <algorithm>
#include <array>
#include <vector>
#include <thread>
#include <iostream>
#include <numeric>
#include <atomic>

struct IUB
{
   float p;
   char c;
};

const std::string alu =
{
   "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGG"
   "GAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGA"
   "CCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAAT"
   "ACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCA"
   "GCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGG"
   "AGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCC"
   "AGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA"
};

std::array<IUB,15> iub =
{{
   { 0.27f, 'a' },
   { 0.12f, 'c' },
   { 0.12f, 'g' },
   { 0.27f, 't' },
   { 0.02f, 'B' },
   { 0.02f, 'D' },
   { 0.02f, 'H' },
   { 0.02f, 'K' },
   { 0.02f, 'M' },
   { 0.02f, 'N' },
   { 0.02f, 'R' },
   { 0.02f, 'S' },
   { 0.02f, 'V' },
   { 0.02f, 'W' },
   { 0.02f, 'Y' }
}};

std::array<IUB, 4> homosapiens =
{{
   { 0.3029549426680f, 'a' },
   { 0.1979883004921f, 'c' },
   { 0.1975473066391f, 'g' },
   { 0.3015094502008f, 't' }
}};

const int IM = 139968;
const float IM_RECIPROCAL = 1.0f / IM;

uint32_t gen_random()
{
   static const int IA = 3877, IC = 29573;
   static int last = 42;
   last = (last * IA + IC) % IM;
   return last;
}

char convert_trivial(char c)
{
   return c;
}

template<class iterator_type>
class repeat_generator_type {
public:
   using result_t = char;
   
   repeat_generator_type(iterator_type first, iterator_type last)
   : first(first), current(first), last(last)
   { }
   result_t operator()()
   {
      if (current == last)
         current = first;
      iterator_type p = current;
      ++current;
      return *p;
   }
private:
   iterator_type first;
   iterator_type current;
   iterator_type last;
};
template<class iterator_type>
repeat_generator_type<iterator_type>
make_repeat_generator(iterator_type first, iterator_type last)
{return repeat_generator_type<iterator_type>(first, last);}

template<class iterator_type>
char convert_random(uint32_t random, iterator_type begin, iterator_type end)
{
   const float p = random * IM_RECIPROCAL;
   auto result = std::find_if(begin, end, [p] (IUB i) { return p <= i.p; });
   return result->c;
}

char convert_IUB(uint32_t random)
{
   return convert_random(random, iub.begin(), iub.end() );
}

char convert_homosapiens(uint32_t random)
{
   return convert_random(random, homosapiens.begin(), homosapiens.end());
}

template<class iterator_type>
class random_generator_type {
public:
   using result_t = uint32_t;
   
   random_generator_type(iterator_type first, iterator_type last)
   : first(first), last(last)
   { }
   result_t operator()()
   {
      return gen_random();
   }
private:
   iterator_type first;
   iterator_type last;
};
template<class iterator_type>
random_generator_type<iterator_type>
make_random_generator(iterator_type first, iterator_type last)
{return random_generator_type<iterator_type>(first, last);}

template<class iterator_type>
void make_cumulative(iterator_type first, iterator_type last)
{
   std::partial_sum(first, last, first,
                [] (IUB l, IUB r) -> IUB { r.p += l.p; return r; });
}

const size_t CHARS_PER_LINE = 60;
const size_t CHARS_PER_LINE_INCL_NEWLINES = CHARS_PER_LINE + 1;
const size_t LINES_PER_BLOCK = 1024;
const size_t VALUES_PER_BLOCK = CHARS_PER_LINE * LINES_PER_BLOCK;
const size_t CHARS_PER_BLOCK_INCL_NEWLINES = CHARS_PER_LINE_INCL_NEWLINES * LINES_PER_BLOCK;

const unsigned THREADS_TO_USE = std::max( 1U, std::min( 4U, std::thread::hardware_concurrency() ));

std::atomic_size_t g_fillThreadIndex = 0;
size_t g_totalValuesToGenerate = 0;

std::mutex g_fillMutex;

std::vector<char> output_buffer;

// Function to flush output buffer to stdout
void flush_output() {
    std::lock_guard<std::mutex> lock(g_fillMutex);
    std::fwrite(output_buffer.data(), 1, output_buffer.size(), stdout);
    output_buffer.clear();
}

// Calling flush every after threads processed their work

#define LOCK( mutex ) std::lock_guard< decltype( mutex ) > guard_{ mutex };

size_t fillBlock(size_t currentThread, std::vector<uint32_t>::iterator begin, random_generator_type<std::array<IUB, 15>::iterator>& generator) {
   while(true) {
      if(currentThread == g_fillThreadIndex.load(std::memory_order_relaxed)) {
         // Select the next thread for this work.
         g_fillThreadIndex.fetch_add(1, std::memory_order_relaxed);
         if(g_fillThreadIndex.load(std::memory_order_relaxed) >= THREADS_TO_USE) {
            g_fillThreadIndex.store(0, std::memory_order_relaxed);
         }

         // Do the work.
         const size_t valuesToGenerate = std::min(g_totalValuesToGenerate, VALUES_PER_BLOCK);
         g_totalValuesToGenerate -= valuesToGenerate;

         for(size_t valuesRemaining = 0; valuesRemaining < valuesToGenerate; ++valuesRemaining) {
            *begin++ = generator();
         }
         
         return valuesToGenerate;
      }
   }
}

templateg<class BlockIter, class converter_type, std::vector<char>::iterator CharIter>
size_t convertBlock(BlockIter begin, BlockIter end, CharIter outCharacter, converter_type& converter) {
   const auto beginCharacter = outCharacter;
   size_t col = 0;
   for(; begin != end; ++begin) {
      const uint32_t random = *begin;

      *outCharacter++ = converter(random);
      if(++col >= CHARS_PER_LINE) {
         col = 0;
         *outCharacter++ = '\n';
      }
   }
   // Check if we need to end the line
   if(0 != col) {
      // Last iteration didn't end the line, so finish the job.
      *outCharacter++ = '\n';
   }
   return std::distance(beginCharacter, outCharacter);
}

std::mutex g_outMutex;
std::atomic_size_t g_outThreadIndex = -1;

void writeCharacters(size_t currentThread, std::vector<char>::iterator begin, size_t count) {
   while(true) {
      if(g_outThreadIndex.load(std::memory_order_relaxed) == -1 || currentThread == g_outThreadIndex.load(std::memory_order_relaxed)) {
         // Select the next thread for this work.
         g_outThreadIndex.store(currentThread + 1, std::memory_order_relaxed);
         if(g_outThreadIndex.load(std::memory_order_relaxed) >= THREADS_TO_USE) {
            g_outThreadIndex.store(0, std::memory_order_relaxed);
         }

         // Do the work.
         {
             LOCK(g_outMutex);
             output_buffer.insert(output_buffer.end(), begin, begin + count);
             if(output_buffer.size() > 1024 * 1024) {
                 flush_output();
             }
         }
         return;
      }
   }
}


void work(size_t currentThread, random_generator_type<std::array<IUB, 15>::iterator>& generator, char(*converter)(uint32_t)) {
   std::vector<uint32_t> block(VALUES_PER_BLOCK);
   std::vector<char> characters(CHARS_PER_BLOCK_INCL_NEWLINES);

   while(true) {
      const auto bytesGenerated = fillBlock(currentThread, block.begin(), generator);

      if(bytesGenerated == 0) {
         break;
      }

      const auto charactersGenerated = convertBlock(block.begin(), block.begin() + bytesGenerated, characters.begin(), converter);

      writeCharacters(currentThread, characters.begin(), charactersGenerated);
   }
   flush_output();   // Ensure final flush 
}

template <class generator_type, class converter_type>
void make(const char* desc, int n, generator_type generator, converter_type converter) {
   std::cout << '>' << desc << '\n';

   g_totalValuesToGenerate = n;
   g_outThreadIndex = -1;
   g_fillThreadIndex.store(0, std::memory_order_relaxed);

   std::vector<std::thread> threads(THREADS_TO_USE - 1);
   for(size_t i = 0; i < threads.size(); ++i) {
      threads[i] = std::thread{
          [&generator, &converter, i]() {
              work(i, generator, std::ref(converter));
          }
      };
   }

   work(threads.size(), generator, converter);

   for(auto& thread : threads) {
      thread.join();
   }
   flush_output();   // Final flush to ensure all output is handled.
}

int main(int argc, char *argv[]) {
   int n = 1000;
   if(argc < 2 || (n = std::atoi(argv[1])) <= 0) {
      std::cerr << "usage: " << argv[0] << " length\n";
      return 1;
   }

   make_cumulative(iub.begin(), iub.end());
   make_cumulative(homosapiens.begin(), homosapiens.end());

   make("ONE Homo sapiens alu", n * 2,
       make_repeat_generator(alu.begin(), alu.end()),
       &convert_trivial);
   make("TWO IUB ambiguity codes", n * 3,
       make_random_generator(iub.begin(), iub.end()),
       &convert_IUB);
   make("THREE Homo sapiens frequency", n * 5,
       make_random_generator(homosapiens.begin(), homosapiens.end()),
       &convert_homosapiens);
   flush_output();
   return 0;
}