#pragma once
/* boost::noncopyable比较简单, 主要用于单例的情况. 通常情况下, 要写一个单例类就要在类的声明把它们的构造函数, 
   赋值函数, 析构函数, 复制构造函数隐藏到private或者protected之中, 每个类都这么做麻烦.有noncopyable类, 
   只要让单例类直接继承noncopyable. */
class noncopyable {
 protected:
  noncopyable() {}
  ~noncopyable() {}

 private:
  noncopyable(const noncopyable&);
  const noncopyable& operator=(const noncopyable&);
};