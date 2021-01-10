#pragma once

/* noncopyable的基本思想是把构造函数和析构函数设置protected权限，这样子类可以调用，但是外面的类不能调用，
 * 那么当子类需要定义构造函数的时候不至于通不过编译。但是最关键的是noncopyable把复制构造函数和复制赋值函数做成了private，
 * 这就意味着除非子类定义自己的copy构造和赋值函数，否则在子类没有定义的情况下，外面的调用者是不能够通过赋值和copy构造等手段
 * 来产生一个新的子类对象的。
 * noncopyable比较简单, 主要用于单例的情况. 通常情况下, 要写一个单例类就要在类的声明把它们的构造函数, 
 * 赋值函数, 析构函数, 复制构造函数隐藏到private或者protected之中, 每个类都这么做麻烦.有noncopyable类, 
 * 只要让单例类直接继承noncopyable. 
 */
class noncopyable {
 protected:
  noncopyable() {}
  ~noncopyable() {}

 private:
  noncopyable(const noncopyable&);
  const noncopyable& operator=(const noncopyable&);
};