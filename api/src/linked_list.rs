use core::{marker::PhantomData, ptr::NonNull};

use alloc::{alloc::{Allocator, Global}, boxed::Box};

pub(crate) struct Node<T>
{
    previous: Option<NonNull<Node<T>>>,
    next: Option<NonNull<Node<T>>>,
    value: T
}
impl<T> Node<T>
{
    const fn new(value: T) -> Self
    {
        return Node { previous: None, next: None, value }
    }
}

pub struct RefNode<T>(NonNull<Node<T>>);
// impl<'a, T> RefNode<'a, T>
// {
//     pub fn as_ref(&'a self) -> &'a T
//     {
//         unsafe
//         {
//             return &self.inner.as_ref().value;
//         }
//     }
// }
pub struct LinkedList<T, A: Allocator = Global>
{
    start: Option<NonNull<Node<T>>>,
    end: Option<NonNull<Node<T>>>,
    alloc: A,
    len: usize
}

impl<'a, T, A: Allocator> Drop for LinkedList<T, A>
{
    fn drop(&mut self)
    {
        // remove previous so that current still exists to iterate with
        for x in self.iter_forward_ref().skip(1)
        {
            unsafe
            {
                // will exist 
                let pre = (*x.0.as_ptr()).previous.unwrap_unchecked();
                // bring back to box so that data is freed
                let _ = Box::from_raw_in(pre.as_ptr(), &self.alloc);
            }
        }
        
        // end is not accounted for
        match self.end
        {
            Some(e) =>
            {
                unsafe
                {
                    // bring back to box so that data is freed
                    let _ = Box::from_raw_in(e.as_ptr(), &self.alloc);
                }
            },
            None => {}
        }
    }
}

impl<'a, T, A: Allocator> LinkedList<T, A>
{
    pub fn append(&'a mut self, value: T) -> RefNode<T>
    {
        let node = Box::new_in(Node::new(value), &self.alloc);
        let node_ptr = NonNull::from(Box::leak(node));
        unsafe
        {
            match self.end
            {
                Some(e) =>
                {
                    (*e.as_ptr()).next = Some(node_ptr);
                    (*node_ptr.as_ptr()).previous = Some(e);
                },
                None =>
                {
                    self.start = Some(node_ptr)
                },
            }
        }
        self.end = Some(node_ptr);
        self.len += 1;
        return RefNode(node_ptr);
    }
    pub fn remove(&'a mut self, rn: RefNode<T>)
    {
        let node_ptr = rn.0;
        unsafe
        {
            let ne = (*node_ptr.as_ptr()).previous;
            let ns = (*node_ptr.as_ptr()).next;
            
            if self.start == Some(node_ptr)
            {
                self.start = ns;
            }
            if self.end == Some(node_ptr)
            {
                self.end = ne;
            }
            
            if let Some(e) = ne
            {
                (*e.as_ptr()).next = ns;
            }
            if let Some(s) = ns
            {
                (*s.as_ptr()).previous = ne;
            }
            
            // bring back to box so that data is freed
            let _ = Box::from_raw_in(node_ptr.as_ptr(), &self.alloc);
        }
    }
    
    /// `order` returns [`true`] if left is strictly less than right
    pub fn insert<O>(&'a mut self, value: T, order: O) -> RefNode<T>
        where O: Fn(&T, &T) -> bool
    {
        // the first node which is greater than the inserting
        let ins = unsafe {
            self.iter_forward_ref().skip_while(|v| order(&v.0.as_ref().value, &value)).nth(0)
        };
        
        match ins
        {
            Some(i) =>
            {
                // insert into list before ins
                unsafe
                {
                    let mut node = Box::new_in(Node::new(value), &self.alloc);
                    let last = (*i.0.as_ptr()).previous;
                    node.previous = last;
                    node.next = Some(i.0);
                    let node_ptr = NonNull::from(Box::leak(node));
                    
                    (*i.0.as_ptr()).previous = Some(node_ptr);
                    
                    match last
                    {
                        Some(l) =>
                        {
                            (*l.as_ptr()).next = Some(node_ptr);
                        },
                        None =>
                        {
                            self.start = Some(node_ptr)
                        },
                    }
                    
                    self.len += 1;
                    return RefNode(node_ptr);
                }
            },
            None =>
            {
                return self.append(value);
            },
        }
    }
    
    #[inline]
    pub fn iter_forward(&'a self) -> impl Iterator<Item = &'a T>
    {
        return IterForward { current: self.start, len: self.len, phantom: PhantomData };
    }
    #[inline]
    pub fn iter_backward(&'a self) -> impl Iterator<Item = &'a T>
    {
        return IterBackward { current: self.start, len: self.len, phantom: PhantomData };
    }
    #[inline]
    pub fn iter_forward_ref(&'a self) -> impl Iterator<Item = RefNode<T>>
    {
        return IterForwardRef { current: self.start, len: self.len, phantom: PhantomData };
    }
    #[inline]
    pub fn iter_backward_ref(&'a self) -> impl Iterator<Item = RefNode<T>>
    {
        return IterBackwardRef { current: self.start, len: self.len, phantom: PhantomData };
    }
    
    #[inline]
    pub fn len(&self) -> usize
    {
        return self.len;
    }
    #[inline]
    pub fn first(&'a self) -> Option<RefNode<T>>
    {
        return self.start.map(|s| RefNode(s));
    }
    #[inline]
    pub fn last(&'a self) -> Option<RefNode<T>>
    {
        return self.end.map(|l| RefNode(l));
    }
    
    #[inline]
    pub fn get_mut(&'a mut self, ref_node: &'a mut RefNode<T>) -> &'a mut T
    {
        unsafe
        {
            return &mut ref_node.0.as_mut().value;
        }
    }
    #[inline]
    pub fn get_ref(&'a self, ref_node: &'a RefNode<T>) -> &'a T
    {
        unsafe
        {
            return &ref_node.0.as_ref().value;
        }
    }
    #[inline]
    pub fn get_next(&'a self, ref_node: &'a RefNode<T>) -> Option<RefNode<T>>
    {
        unsafe
        {
            return ref_node.0.as_ref().next.map(|v| RefNode(v));
        }
    }
    #[inline]
    pub fn get_previous(&'a self, ref_node: &'a RefNode<T>) -> Option<RefNode<T>>
    {
        unsafe
        {
            return ref_node.0.as_ref().previous.map(|v| RefNode(v));
        }
    }
}

impl<'a, T: PartialOrd, A: Allocator> LinkedList<T, A>
{
    #[inline]
    pub fn insert_ord(&'a mut self, value: T) -> RefNode<T>
    {
        return self.insert(value, |l, r| l < r);
    }
}

// ITERATORS

struct IterForward<'a, T>
{
    current: Option<NonNull<Node<T>>>,
    len: usize,
    phantom: PhantomData<&'a T>
}
impl<'a, T> Iterator for IterForward<'a, T>
{
    type Item = &'a T;

    fn next(&mut self) -> Option<Self::Item>
    {
        return match self.current
        {
            Some(c) =>
            {
                unsafe
                {
                    self.current = (*c.as_ptr()).next;
                    self.len -= 1;
                    Some(&c.as_ref().value)
                }
            },
            None => None
        };
    }
    
    fn size_hint(&self) -> (usize, Option<usize>)
    {
        return (self.len, Some(self.len));
    }
    fn count(self) -> usize
        where
            Self: Sized
    {
        return self.len;
    }
}
struct IterBackward<'a, T>
{
    current: Option<NonNull<Node<T>>>,
    len: usize,
    phantom: PhantomData<&'a T>
}
impl<'a, T> Iterator for IterBackward<'a, T>
{
    type Item = &'a T;

    fn next(&mut self) -> Option<Self::Item>
    {
        return match self.current
        {
            Some(c) =>
            {
                unsafe
                {
                    self.current = (*c.as_ptr()).previous;
                    self.len -= 1;
                    Some(&c.as_ref().value)
                }
            },
            None => None
        };
    }
    
    fn size_hint(&self) -> (usize, Option<usize>)
    {
        return (self.len, Some(self.len));
    }
    fn count(self) -> usize
        where
            Self: Sized
    {
        return self.len;
    }
}

struct IterForwardRef<'a, T>
{
    current: Option<NonNull<Node<T>>>,
    len: usize,
    phantom: PhantomData<&'a T>
}
impl<'a, T> Iterator for IterForwardRef<'a, T>
{
    type Item = RefNode<T>;

    fn next(&mut self) -> Option<Self::Item>
    {
        return match self.current
        {
            Some(c) =>
            {
                unsafe
                {
                    self.current = (*c.as_ptr()).next;
                    self.len -= 1;
                    Some(RefNode(c))
                }
            },
            None => None
        };
    }
    
    fn size_hint(&self) -> (usize, Option<usize>)
    {
        return (self.len, Some(self.len));
    }
    fn count(self) -> usize
        where
            Self: Sized
    {
        return self.len;
    }
}
struct IterBackwardRef<'a, T>
{
    current: Option<NonNull<Node<T>>>,
    len: usize,
    phantom: PhantomData<&'a T>
}
impl<'a, T> Iterator for IterBackwardRef<'a, T>
{
    type Item = RefNode<T>;

    fn next(&mut self) -> Option<Self::Item>
    {
        return match self.current
        {
            Some(c) =>
            {
                unsafe
                {
                    self.current = (*c.as_ptr()).previous;
                    self.len -= 1;
                    Some(RefNode(c))
                }
            },
            None => None
        };
    }
    
    fn size_hint(&self) -> (usize, Option<usize>)
    {
        return (self.len, Some(self.len));
    }
    fn count(self) -> usize
        where
            Self: Sized
    {
        return self.len;
    }
}