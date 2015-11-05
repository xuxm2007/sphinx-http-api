主要优点：
  1. 本程序主要目的是为了方便查询和调试：拼字符串比调用API容易点，不受语言限制，更方便使用，你可以从浏览器中直接查看搜索结果。
  1. 不用太担心性能，这个程序只是做一些API翻译工作，比较轻松的达到并发1000，对接大部分应用来说都是足够的。
  1. 内部实现了线程池，线程和搜索searchd直接实现长连接，比较容易解决sphinx fork模式的长连接问题，也就是你不用关心长连接问题了。


---


**安装需要Linux C++**

**Sphinx全文搜索引擎的HTTP访问接口，可以通过浏览器直接访问搜索功能**

**当前支持sphinx0.9版client的api,但也可以连现在的sphinx2.0版本**

**GET方式请求,所有查询都基于此,自定义的请求格式**

**请求格式：
> http://ip:port/search?index=mysql&q=%E5%B0%91%E7%94%B7&filter=&start=0&rows=20&matchmode=SPH_MATCH_EXTENDED2&rankingmode=SPH_RANK_PROXIMITY_BM25&sortmode=SPH_SORT_RELEVANCE**

**返回结果json:
```
  {
   "matchNumb" : 20,
   "matches" : [
      {
         "date_added" : 1330952723,
         "group_id" : 463,
         "id" : 243
      },
      {
         "date_added" : 1331434709,
         "group_id" : 931,
         "id" : 567
      },
      {
         "date_added" : 1331822553,
         "group_id" : 382,
         "id" : 817
      },
      {
         "date_added" : 1331314537,
         "group_id" : 233,
         "id" : 1353
      },
      {
         "date_added" : 1331467793,
         "group_id" : 188,
         "id" : 2690
      },
      {
         "date_added" : 1330987804,
         "group_id" : 106,
         "id" : 4984
      },
      {
         "date_added" : 1331685828,
         "group_id" : 655,
         "id" : 5382
      },
      {
         "date_added" : 1331009879,
         "group_id" : 750,
         "id" : 6031
      },
      {
         "date_added" : 1331024905,
         "group_id" : 102,
         "id" : 7827
      },
      {
         "date_added" : 1330952723,
         "group_id" : 463,
         "id" : 10243
      },
      {
         "date_added" : 1331434709,
         "group_id" : 931,
         "id" : 10567
      },
      {
         "date_added" : 1331822553,
         "group_id" : 382,
         "id" : 10817
      },
      {
         "date_added" : 1331314537,
         "group_id" : 233,
         "id" : 11353
      },
      {
         "date_added" : 1331467793,
         "group_id" : 188,
         "id" : 12690
      },
      {
         "date_added" : 1330987804,
         "group_id" : 106,
         "id" : 14984
      },
      {
         "date_added" : 1331685828,
         "group_id" : 655,
         "id" : 15382
      },
      {
         "date_added" : 1331009879,
         "group_id" : 750,
         "id" : 16031
      },
      {
         "date_added" : 1331024905,
         "group_id" : 102,
         "id" : 17827
      },
      {
         "date_added" : 1330952723,
         "group_id" : 463,
         "id" : 20243
      },
      {
         "date_added" : 1331434709,
         "group_id" : 931,
         "id" : 20567
      }
   ],
   "status" : 0,
   "status_code" : "[0,1,2,3]",
   "status_info" : "[SEARCHD_OK,SEARCHD_ERROR,SEARCHD_RETRY,SEARCHD_WARNING]",
   "time" : 137,
   "total" : 20,
   "total_found" : 678,
   "wordNumb" : 1,
   "words" : [
      {
         "docs" : 27,
         "hits" : 27,
         "word" : "少男"
      }
   ]
 }
```

---** 具体语法看doc目录的使用手册

**同时提供：**

**/status sphinx的状态信息json**

**/statistic proxy的统计信息json**

**/web/info.html 简介页面**

