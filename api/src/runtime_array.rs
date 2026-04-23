use core::ops::{Index, IndexMut};

pub struct RA<T, const CAP: usize>
    where [(); CAP * core::mem::size_of::<T>()]: Sized
{
    inner: [u8; CAP * core::mem::size_of::<T>()],
    len: usize
}

impl<T, const CAP: usize> AsRef<[T]> for RA<T, CAP>
    where [(); CAP * core::mem::size_of::<T>()]: Sized
{
    fn as_ref(&self) -> &[T]
    {
        unsafe
        {
            return core::mem::transmute::<_, &[T]>(&self.inner[0..self.len]);
        }
    }
}
impl<T, const CAP: usize> AsMut<[T]> for RA<T, CAP>
    where [(); CAP * core::mem::size_of::<T>()]: Sized
{
    fn as_mut(&mut self) -> &mut [T]
    {
        unsafe
        {
            return core::mem::transmute::<_, &mut [T]>(&mut self.inner[0..self.len]);
        }
    }
}

impl<T, const CAP: usize> IndexMut<usize> for RA<T, CAP>
    where [(); CAP * core::mem::size_of::<T>()]: Sized
{
    fn index_mut(&mut self, index: usize) -> &mut Self::Output
    {
        return self.as_mut().index_mut(index);
    }
}

impl<T, const CAP: usize> Index<usize> for RA<T, CAP>
    where [(); CAP * core::mem::size_of::<T>()]: Sized
{
    type Output = T;

    fn index(&self, index: usize) -> &Self::Output
    {
        return self.as_ref().index(index);
    }
}

impl<T, const CAP: usize> RA<T, CAP>
    where [(); CAP * core::mem::size_of::<T>()]: Sized
{
    pub fn from_iter(iter: impl Iterator<Item = T>) -> Self
    {
        let mut inner = [0u8; CAP * core::mem::size_of::<T>()];
        let slice = unsafe { core::mem::transmute::<_, &mut [T; CAP]>(&mut inner) };
        
        let mut count = 0;
        for (s, d) in iter.zip(slice)
        {
            *d = s;
            count += 1;
        }
        
        return Self { inner, len: count };
    }
    
    #[inline]
    pub fn len(&self) -> usize
    {
        return self.len;
    }
}

impl<T: Copy, const CAP: usize> RA<T, CAP>
    where [(); CAP * core::mem::size_of::<T>()]: Sized
{
    pub fn new(value: T, size: usize) -> Self
    {
        let mut inner = [0u8; CAP * core::mem::size_of::<T>()];
        let slice = unsafe { core::mem::transmute::<_, &mut [T; CAP]>(&mut inner) };
        *slice = [value; CAP];
        
        return Self { inner, len: size };
    }
}

impl<T: Default, const CAP: usize> Default for RA<T, CAP>
    where [(); CAP * core::mem::size_of::<T>()]: Sized
{
    fn default() -> Self
    {
        let inner = [0u8; CAP * core::mem::size_of::<T>()];
        return Self { inner, len: 0 };
    }
}
