/**
 * SearchResult.h
 * Copyright 2012 zhaigy hontlong@gmail.com
 */

#ifndef SRC_SEARCH_RESULT_H_
#define SRC_SEARCH_RESULT_H_


class SearchResult {
  public:
    // 当查询出错时，存在res为NULL的情况
    sphinx_result * res_;
    sphinx_client * client_;

  public:
    SearchResult() : res_(NULL), client_(NULL) {
    }

    SearchResult(sphinx_client * const client, sphinx_result * const res) {
      this->client_ = client;
      this->res_ = res;
    }

    SearchResult(const SearchResult & other) {
      this->client_ = other.client_;
      this->res_ = other.res_;
    }

    void set_result(sphinx_result * res) {
      this->res_ = res;
    }

    void set_client(sphinx_client * client) {
      this->client_ = client;
    }

    sphinx_client * get_client( ) { return this->client_;}

    int getStatus() const {
      if (res_ == NULL) {
        return SEARCHD_ERROR;
      }
      return res_->status;
    }

    unsigned int getMatchNumb() const {
      return (unsigned int) res_->num_matches;
    }

    unsigned int getWordNumb() const {
      return (unsigned int) res_->num_words;
    }

    unsigned int getTotal() const {
      return (unsigned int) res_->total;
    }

    unsigned int getTime() const {
      return (unsigned int) res_->time_msec;
    }

    const char * getError() const {
      return sphinx_error(client_);
    }

    const char * getWarning() const {
      return sphinx_warning(client_);
    }

    sphinx_uint64_t getId(unsigned int idx) const {
      return sphinx_get_id(res_, idx);
    }

    unsigned int getAttrNumb() const {
      return (unsigned int) res_->num_attrs;
    }

    const char * getAttrName(unsigned int attrIdx) const {
      return res_->attr_names[attrIdx];
    }

    unsigned int getAttrType(unsigned int attrIdx) const {
      return res_->attr_types[attrIdx];
    }

    unsigned int getAttrIntValue(unsigned int idx, unsigned int attrIdx) const {
      int value = sphinx_get_int(res_, idx, attrIdx);
      return (unsigned int) value;
    }

    float getAttrFloatValue(unsigned int idx, unsigned int attrIdx) const {
      float value = sphinx_get_float(res_, idx, attrIdx);
      return value;
    }

    unsigned int * getAttrMultiIntValue(unsigned int idx, unsigned int attrIdx)
      const {
        unsigned int * mva = sphinx_get_mva(res_, idx, attrIdx);
        return mva;
      }

    const char * getWord(unsigned int wordIdx) const {
      return res_->words[wordIdx].word;
    }

    unsigned int getHits(unsigned int wordIdx) const {
      return res_->words[wordIdx].hits;
    }

    unsigned int getDocs(unsigned int wordIdx) const {
      return res_->words[wordIdx].docs;
    }
};

#endif  // SRC_SEARCH_RESULT_H_

