#pragma once
#include <algorithm>
#include <iostream>

template <typename Iterator>
struct IteratorRange {
    Iterator begin_;
    Iterator end_;
    IteratorRange (Iterator begin, Iterator end): begin_(begin), end_(end){} 
};

template <typename Iterator>
class Paginator {
public:
Paginator (Iterator range_begin, Iterator range_end, int page_size)
    : page_size_(page_size)
{

Iterator temp = range_begin;
for (; temp + page_size < range_end; temp += page_size){
    pages.push_back(IteratorRange(temp, temp + page_size));
    
}
if (temp < range_end) pages.push_back(IteratorRange(temp, range_end));

}

auto begin() const {
    return pages.begin();
  }
  
auto end() const
  {
    return pages.end();
  }
  
int size() const
    {
    return page_size_;
    }

private:
std::vector<IteratorRange<Iterator>> pages;
int page_size_;
    
};

template <typename Type>
std::ostream& operator<<(std::ostream& out, IteratorRange<Type> search){
    for (auto i = search.begin_; i < search.end_; i++){
         out << *i;
    }
   return out;
}

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}
