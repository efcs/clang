
namespace __secret {
struct my_t {};
} // namespace __secret

void t1() { my_t t; }

namespace std {
struct string {};
int __invoke(string);
} // namespace std

void t2() {
  std::string s;
  int x = std::invoke(s);
}

namespace std {
inline namespace __1 {
struct vector {};

using container = vector;
} // namespace __1
} // namespace std
void t3() { container r; }
