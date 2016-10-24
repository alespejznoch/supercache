from django.http import HttpResponse
from django.views.decorators.cache import cache_page
from django.core.cache import cache

view_ttl = 30


cache.set('a', 1)
cache.set('b', 2)
cache.set('c', 3,20)
cache.set('d', 4)
cache.set('e', 5)
cache.set('f', 6)
# no g
cache.delete('b')
cache.delete_many([ 'b', 'e', 'd'])


cache_all = cache.get_many(['a', 'b', 'c', 'd', 'e', 'f', 'g'])
""" should contain a,c,f """
print("cache contains a,c,f:")
print(cache_all)

cache.clear()
cache_all = cache.get_many(['a', 'b', 'c', 'd', 'e', 'f', 'g'])
""" should be empty """
print("cache is empty:")
print(cache_all)

cache.set('house', 'mouse',10)

""" set again """
cache.set('a', 1)
cache.set('b', 2)
cache.set('c', 3,20)
cache.set('d', 4)
cache.set('e', 5)
cache.set('f', 6)
cache.delete('b')
cache.delete_many([ 'b', 'e', 'd'])

@cache_page(view_ttl, key_prefix="cachetest")
def index(request):
    
    cache_all = cache.get_many(['a', 'b', 'c', 'd', 'e', 'f', 'g'])
    print("should print a,c,f, after 20s, c will be deleted, but the view is cached for 30s:")
    print(cache_all)



    # 0 timeout means no caching
    cache.set('house', 'dog', 0)
    cache.add('house', 'cat')
    print("house is occupied by a mouse,")
    print("after 10s, house will be occupied by a cat ")
    print(cache.get('house'))
    

    content = "This view is cached for " +str(view_ttl)+ " seconds.</br>";
    return HttpResponse(content)
