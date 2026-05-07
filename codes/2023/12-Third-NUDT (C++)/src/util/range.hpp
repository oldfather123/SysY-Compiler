#ifndef __RANGE_HPP__
#define __RANGE_HPP__

namespace Util {

template <typename Iterator>
class Range {
private:
  Iterator _begin;
  Iterator _end;

public:
  explicit Range(Iterator begin, Iterator end) : _begin(begin), _end(end) {
  }
  Iterator begin() {
    return _begin;
  }
  Iterator end() {
    return _end;
  }
  size_t size() const {
    return std::distance(_begin, _end);
  }
  bool empty() const {
    return _begin == _end;
  }
};

template <typename Iterator>
Range<Iterator> range(Iterator begin, Iterator end) {
  return Range(begin, end);
}

template <typename Container>
Range<typename Container::iterator> range(Container &_container) {
  return range(_container.begin(), _container.end());
}

template <typename Container>
Range<typename Container::const_iterator> range(const Container &_container) {
  return range(_container.begin(), _container.end());
}

}  // namespace Util

#endif
