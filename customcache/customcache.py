"Thread-safe in-memory super cache backend!"

import time
from contextlib import contextmanager

from django.core.cache.backends.base import DEFAULT_TIMEOUT, BaseCache
from django.utils.synch import RWLock

""" import C++ Super cache"""
from . import supercache

_caches = {}


@contextmanager
def dummy():
    """A context manager that does nothing special."""
    yield

class SuperCache(BaseCache):
    def __init__(self, name, params):
        BaseCache.__init__(self, params)
        self._cache = _caches.setdefault(name, {})



    """
    Set a value in the cache if the key does not already exist. If
    timeout is given, that timeout will be used for the key; otherwise
    the default cache timeout will be used.

    Returns True if the value was stored, False otherwise.
    """
    def add(self, key, value, timeout=DEFAULT_TIMEOUT, version=None):
        if self.has_key(key):
            return False
        self.set(key, value, timeout)
        return True

    """
    Fetch a given key from the cache. If the key does not exist, return
    default, which itself defaults to None.
    """
    def get(self, key, default=None, version=None):
        key = self.make_key(key, version=version)
        return supercache.get(key, default)

    """
    Set a value in the cache. If timeout is given, that timeout will be
    used for the key; otherwise the default cache timeout will be used.
    """
    def set(self, key, value, timeout=DEFAULT_TIMEOUT, version=None):
        key = self.make_key(key, version=version)
        supercache.set(key, value, timeout)

    """
    Delete a key from the cache, failing silently.
    """
    def delete(self, key, version=None):
        key = self.make_key(key, version=version)
        supercache.delete(key)

    """Remove all values from the cache at once."""
    def clear(self):
        supercache.clear()

    """
    Returns True if the key is in the cache and has not expired or was not removed.
    """
    def has_key(self, key, version=None):
        return self.get(key, None) is not None
