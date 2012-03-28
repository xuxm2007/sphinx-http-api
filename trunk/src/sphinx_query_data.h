/**
 * SphinxQueryData.h
 *
 *  Copyright 2011 zhaigy hontlong@gmail.com
 */
#ifndef SRC_SPHINX_QUERY_DATA_H_
#define SRC_SPHINX_QUERY_DATA_H_

#include <string>
#include <numeric>
#include <cstdlib>
#include <list>
#include <vector>
#include <functional>
#include <algorithm>
#include "./utils.h"

using namespace std;  // NOLINT

/**
 * 从输入解析的查询数据
 * FIXME: 预备去除
 */
class QueryData {
  public:
    string index;
    string q;
    list<string> filters;
    string idrange;
    string start;
    string rows;
    string fieldweights;
    string retries;
    string maxquerytime;
    string connecttimeout;
    string groupby;
    string groupdistinct;
    string matchmode;
    string rankingmode;
    string sortmode;
    string select;
};

/**
 * 可以应用于Sphinx接口的查询数据
 */
class SphinxQueryData {
  public:
    struct Filter {
      Filter() {
        exclude = false;
      }

      ~Filter() {
      }

      enum Type {
        kValue, kIntRange, kFloatRange
      };

      Type type;
      string attr_name;
      bool exclude;
      vector<sphinx_int64_t> values;
      union {
        int int_min;
        float float_min;
      };
      union {
        int int_max;
        float float_max;
      };
    };

    // FIXME: 增强输入参数的格式检查
    explicit SphinxQueryData(QueryData qd) {
#define THROW_F_E(key, str) \
      do { throw string("格式错误:[") + (#key) + "=" + (str) + "]"; } while (0)
#define STR_TO_NUMB(name) \
      do { \
        if (qd.name.empty()) break; \
        if (qd.name.size() > 9 || !all_is_digit(qd.name)) { \
          THROW_F_E(name, qd.name); \
        } \
        name = atoi(qd.name.c_str()); \
        if (name < 0) { \
          THROW_F_E(name, qd.name); \
        } \
      } while (0)

      start = 0;
      rows = 20;
      retries = 1;
      maxquerytime = 3000;
      connecttimeout = 1000;

      STR_TO_NUMB(start);
      STR_TO_NUMB(rows);
      STR_TO_NUMB(retries);
      STR_TO_NUMB(maxquerytime);
      STR_TO_NUMB(connecttimeout);
#undef STR_TO_NUMB

      // 初始默认值
      idrange_min = -1L;
      idrange_max = -1L;

      fieldweights_num = 0;
      fieldweights_fields = NULL;
      fieldweights_weights = NULL;

      groupby_func = SPH_GROUPBY_ATTR;

      matchmode = SPH_MATCH_EXTENDED2;
      rankingmode = SPH_RANK_PROXIMITY_BM25;
      sortmode = SPH_SORT_RELEVANCE;

      // 查询的二次转换，细节解析和检查
      index = qd.index;
      q = qd.q;
      // 过滤器 filter - 属性值，属性范围，属性范围float
      {
        // filter=attr:a,b,c,d
        // filter=!attr:a,b,c,d
        // filter=attr:[a TO b]
        // filter=!attr:[a TO b]
        // 过滤器仅支持上述4种写法不支持小括号
        for (list<string>::iterator it = qd.filters.begin();
              it != qd.filters.end(); ++it) {
          Filter filter;
          if (it->at(0) == '!') filter.exclude = true;
          string::size_type s = filter.exclude ? 1 : 0;
          string::size_type p = it->find(':', s+1);
          if (p == string::npos || p == s) {
            THROW_F_E(filter, *it);
          }
          filter.attr_name = it->substr(s, p - s);
          ++p;
          p = it->find_first_not_of(" ", p);
          if (p == string::npos) {
            THROW_F_E(filter, *it);
          }
          if (it->at(p) == '[' || it->at(p) == '(') {
            // inner range | outer range
            bool from_is_inner = true;
            bool to_is_inner = true;
            if (it->at(p) == '(') from_is_inner = false;
            ++p;
            p = it->find_first_not_of(" ", p);
            if (p == string::npos) {
              THROW_F_E(filter, *it);
            }
            s = p;
            const char * TO = " TO ";
            p = it->find(TO, s);
            if (p == string::npos) {
              THROW_F_E(filter, *it);
            }
            string num_from = it->substr(s, p - s);
            trim_right(num_from);
            if (num_from.find_first_not_of(".0123456789") != string::npos) {
              THROW_F_E(filter, *it);
            }
            p += strlen(TO);  // skip TO
            s = p;
            p = it->find_first_of("])", s);
            if (p == string::npos || p == s) {
              THROW_F_E(filter, *it);
            }
            if ((p + 1) != it->size()) {
              // 前面已经Trim过了，这里的范围结束符一定要在最后
              THROW_F_E(filter, *it);
            }
            if (it->at(p) == ')') to_is_inner = false;
            string num_to = it->substr(s, p - s);
            trim(num_to);
            if (num_to.find_first_not_of(".0123456789") != string::npos) {
              THROW_F_E(filter, *it);
            }
            filter.type = SphinxQueryData::Filter::kIntRange;
            if (num_from.find('.') != string::npos ||
                  num_to.find('.') != string::npos) {
              filter.type = SphinxQueryData::Filter::kFloatRange;
            }

            if (filter.type == SphinxQueryData::Filter::kIntRange) {
              filter.int_min = atoi(num_from.c_str());
              filter.int_max = atoi(num_to.c_str());
              if (!from_is_inner) filter.int_min += 1;
              if (!to_is_inner) filter.int_max -= 1;
              if (filter.int_min > filter.int_max) {
                THROW_F_E(filter, *it);
              }
            } else {
              // float
              filter.float_min = atof(num_from.c_str());
              filter.float_max = atof(num_to.c_str());
              if (!from_is_inner) filter.float_min += 0.00001;
              if (!to_is_inner) filter.float_max -= 0.00001;
              if (filter.float_min + 0.00001 > filter.float_max) {
                THROW_F_E(filter, *it);
              }
            }
          } else {
            // values list
            // filter=attr:a,b,c,d
            // filter=!attr:a,b,c,d
            filter.type = SphinxQueryData::Filter::kValue;
            s = p;  // 第一个非空值
            for (; true; s = p + 1) {
              p = it->find(',', s);
              if (p != string::npos) {
                if (p <= s) {
                  THROW_F_E(filter, *it);
                }
              }
              string temp_value = it->substr(s, p == string::npos?p:(p - s));
              trim(temp_value);
              if (!all_is_digit(temp_value) || temp_value.size() > 9) {
                THROW_F_E(filter, *it);
              }

              filter.values.push_back(atoi(temp_value.c_str()));
              if (p == string::npos) break;
            }
          }
          // 过滤器组装完毕
          filters.push_back(filter);
        }
      }  // ~filter

      // idrange
      // idrange_min = 0L;
      // idrange_max = 0L;
      if (qd.idrange.size() > 0) {
        string::size_type p = qd.idrange.find_first_of(',', 0);
        if (p != string::npos) {
          string min_str = qd.idrange.substr(0, p);
          string max_str = qd.idrange.substr(p + 1);
          if (!all_is_digit(min_str) || min_str.size() > 9
                || !all_is_digit(max_str) || max_str.size() > 9) {
            THROW_F_E(idrange, qd.idrange);
          }
          idrange_min = atol(min_str.c_str());
          idrange_max = atol(max_str.c_str());
        }
      }

      // fieldweight - 字段数，字段名数组，权重数组（整数）
      // fieldweights=fieldName1:1,fieldName2:5
      // fieldweights = qd.fieldweights;
      fieldweights_num = 0;
      fieldweights_fields = NULL;
      fieldweights_weights = NULL;
      if (qd.fieldweights.size() > 0) {
        fieldweights_num = count_if(qd.fieldweights.begin(),
            qd.fieldweights.end(), bind1st(equal_to<char>(), ','));
        fieldweights_fields = new const char*[fieldweights_num];
        fieldweights_weights = new int[fieldweights_num];

        string::size_type s = 0;
        string::size_type e = string::npos;
        for (int i = 0; i < fieldweights_num; i++) {
          e = qd.fieldweights.find_first_of(',', s);

          string::size_type p = qd.fieldweights.find_first_of(':', s);
          if (p == string::npos) {
            THROW_F_E(fieldweights, qd.fieldweights);
          }
          string field_name = qd.fieldweights.substr(s, p - s);
          trim(field_name);
          if (field_name.empty()) {
            THROW_F_E(fieldweights, qd.fieldweights);
          }
          char * p_field_temp = new char[field_name.size() + 1];
          strncpy(p_field_temp, field_name.c_str(), field_name.size());
          p_field_temp[field_name.size()] = 0;
          fieldweights_fields[i] = p_field_temp;
          string weight;
          if (e != string::npos) {
            weight = qd.fieldweights.substr(p + 1, e - (p + 1));
          } else {
            weight = qd.fieldweights.substr(p + 1);
          }
          trim(weight);
          if (!all_is_digit(weight) || weight.size() > 9) {
            THROW_F_E(fieldweights, qd.fieldweights);
          }
          fieldweights_weights[i] = atoi(weight.c_str());
          if (fieldweights_weights[i] <= 0) {
            THROW_F_E(fieldweights, qd.fieldweights);
          }
        }
      }

      // groupby
      // groupby= SPH_GROUPBY_ATTR:attrName:@group desc.
      groupby = qd.groupby;
      groupby_func = SPH_GROUPBY_ATTR;
      groupby_attr = "";  // 用于决定是否设定了
      groupby_sort = "@group desc";
      if (qd.groupby.size() > 0) {
        string::size_type split1 = qd.groupby.find(':', 0);
        if (split1 != string::npos) {
          string func_name = qd.groupby.substr(0, split1);
          trim(func_name);
          if (func_name == "SPH_GROUPBY_DAY") groupby_func = SPH_GROUPBY_DAY;
          else if (func_name == "SPH_GROUPBY_WEEK") {
            groupby_func = SPH_GROUPBY_WEEK;
          } else if (func_name == "SPH_GROUPBY_MONTH") {
            groupby_func = SPH_GROUPBY_MONTH;
          } else if (func_name == "SPH_GROUPBY_YEAR") {
            groupby_func = SPH_GROUPBY_YEAR;
          } else if (func_name == "SPH_GROUPBY_ATTR") {
            groupby_func = SPH_GROUPBY_ATTR;
          } else {
            throw string("groupby错误的格式:[")+qd.groupby+"]";
          }

          string::size_type split2 = qd.groupby.find(':', split1 + 1);
          if (split2 != string::npos) {
            groupby_attr = qd.groupby.substr(split1 + 1, split2 - (split1 + 1));
            groupby_sort = qd.groupby.substr(split2 + 1);
          } else {
            if (groupby_func == SPH_GROUPBY_ATTR) {
              // 必须指定第三个参数
              throw string("groupby错误的格式:[")+qd.groupby+"]";
            }
            groupby_attr = qd.groupby.substr(split1 + 1);
          }
        }
      }

      groupdistinct = qd.groupdistinct;

      // 匹配模式
      if (qd.matchmode == "SPH_MATCH_ALL") {
        matchmode = SPH_MATCH_ALL;
      } else if (qd.matchmode == "SPH_MATCH_ANY") {
        matchmode = SPH_MATCH_ANY;
      } else if (qd.matchmode == "SPH_MATCH_PHRASE") {
        matchmode = SPH_MATCH_PHRASE;
      } else if (qd.matchmode == "SPH_MATCH_BOOLEAN") {
        matchmode = SPH_MATCH_BOOLEAN;
      } else if (qd.matchmode == "SPH_MATCH_EXTENDED") {
        matchmode = SPH_MATCH_EXTENDED;
      } else if (qd.matchmode == "SPH_MATCH_FULLSCAN") {
        matchmode = SPH_MATCH_FULLSCAN;
      } else if (qd.matchmode == "SPH_MATCH_EXTENDED2") {
        matchmode = SPH_MATCH_EXTENDED2;
      } else if (qd.matchmode.empty()) {
        // 默认值
      } else {
        throw string("matchmode is error![") + qd.matchmode + "]";
      }

      // 评分方式
      if (qd.rankingmode == "SPH_RANK_PROXIMITY_BM25") {
        rankingmode = SPH_RANK_PROXIMITY_BM25;
      } else if (qd.rankingmode == "SPH_RANK_BM25") {
        rankingmode = SPH_RANK_BM25;
      } else if (qd.rankingmode == "SPH_RANK_NONE") {
        rankingmode = SPH_RANK_NONE;
      } else if (qd.rankingmode == "SPH_RANK_WORDCOUNT") {
        rankingmode = SPH_RANK_WORDCOUNT;
      } else if (qd.rankingmode.empty()) {
        // 默认值
      } else {
        throw string("rankingmode is error![") + qd.rankingmode + "]";
      }

      // 排序模式
      string strSortMode;
      string::size_type loc1 = qd.sortmode.find(":", 0);
      if (loc1 == string::npos) {
        strSortMode = qd.sortmode;
        sortclause = "";
      } else {
        strSortMode = qd.sortmode.substr(0, loc1);
        sortclause = qd.sortmode.substr(loc1 + 1);
      }

      if (strSortMode == "SPH_SORT_RELEVANCE") {
        sortmode = SPH_SORT_RELEVANCE;
      } else if (strSortMode == "SPH_SORT_ATTR_DESC") {
        sortmode = SPH_SORT_ATTR_DESC;
      } else if (strSortMode == "SPH_SORT_ATTR_ASC") {
        sortmode = SPH_SORT_ATTR_ASC;
      } else if (strSortMode == "SPH_SORT_TIME_SEGMENTS") {
        sortmode = SPH_SORT_TIME_SEGMENTS;
      } else if (strSortMode == "SPH_SORT_EXTENDED") {
        sortmode = SPH_SORT_EXTENDED;
      } else if (strSortMode == "SPH_SORT_EXPR") {
        sortmode = SPH_SORT_EXPR;
      } else if (strSortMode.empty()) {
        // 默认值
      } else {
        throw "strSortMode is error![" + strSortMode + "]";
      }
      select = qd.select;
#undef THROW_F_E
    }

    ~SphinxQueryData() {
      if (fieldweights_weights != NULL) {
        delete fieldweights_weights;
      }
      if (fieldweights_fields != NULL) {
        for (int i = 0; i < fieldweights_num; i++) {
          if (fieldweights_fields[i] != NULL) {
            delete fieldweights_fields[i];
          }
        }
        delete fieldweights_fields;
      }
    }

    // fields

    string index;
    string q;

    list<Filter> filters;

    int start;
    int rows;

    int64_t idrange_min;
    int64_t idrange_max;
    // fieldweight - 字段数，字段名数组，权重数组（整数）
    // string fieldweights;
    int fieldweights_num;
    const char ** fieldweights_fields;
    int * fieldweights_weights;

    int retries;
    int maxquerytime;
    int connecttimeout;
    string groupby;
    int groupby_func;
    string groupby_attr;  // 用于决定是否设定了
    string groupby_sort;
    string groupdistinct;
    int matchmode;
    int rankingmode;
    int sortmode;
    string sortclause;
    string select;

    static const int kDEF_MATCHMODE = SPH_MATCH_EXTENDED2;
    static const int kDEF_RANKINGMODE = SPH_RANK_PROXIMITY_BM25;
    static const int kDEF_SORTMODE = SPH_SORT_RELEVANCE;
};

#endif  // SRC_SPHINX_QUERY_DATA_H_
