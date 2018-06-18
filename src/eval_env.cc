// Copyright 2011 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <assert.h>

#include "eval_env.h"

string Env::LookupVariable(const string& var) {
  string result;
  string err;
  AppendVariable(var, &result, &err);
  return result;
}

bool BindingEnv::AppendVariable(const string& var, string* result, string* err) {
  map<string, string>::iterator i = bindings_.find(var);
  if (i != bindings_.end()) {
    result->append(i->second);
    return true;
  }

  if (parent_)
    return parent_->AppendVariable(var, result, err);

  return true;
}

void BindingEnv::AddBinding(const string& key, const string& val) {
  bindings_[key] = val;
}

void BindingEnv::AddRule(const Rule* rule) {
  assert(LookupRuleCurrentScope(rule->name()) == NULL);
  rules_[rule->name()] = rule;
}

const Rule* BindingEnv::LookupRuleCurrentScope(const string& rule_name) {
  map<string, const Rule*>::iterator i = rules_.find(rule_name);
  if (i == rules_.end())
    return NULL;
  return i->second;
}

const Rule* BindingEnv::LookupRule(const string& rule_name) {
  map<string, const Rule*>::iterator i = rules_.find(rule_name);
  if (i != rules_.end())
    return i->second;
  if (parent_)
    return parent_->LookupRule(rule_name);
  return NULL;
}

void Rule::AddBinding(const string& key, const EvalString& val) {
  bindings_[key] = val;
}

const EvalString* Rule::GetBinding(const string& key) const {
  Bindings::const_iterator i = bindings_.find(key);
  if (i == bindings_.end())
    return NULL;
  return &i->second;
}

// static
bool Rule::IsReservedBinding(const string& var) {
  return var == "command" ||
      var == "depfile" ||
      var == "description" ||
      var == "deps" ||
      var == "generator" ||
      var == "pool" ||
      var == "restat" ||
      var == "rspfile" ||
      var == "rspfile_content" ||
      var == "msvc_deps_prefix";
}

const map<string, const Rule*>& BindingEnv::GetRules() const {
  return rules_;
}

bool BindingEnv::AppendWithFallback(const string& var,
                                    const EvalString* eval,
                                    Env* env,
                                    string* result,
                                    string* err) {
  map<string, string>::iterator i = bindings_.find(var);
  if (i != bindings_.end()) {
    result->append(i->second);
    return true;
  }

  else if (eval)
    return eval->EvalAppend(env, result, err);

  else if (parent_)
    return parent_->AppendVariable(var, result, err);

  return true;
}

string EvalString::Evaluate(Env* env) const {
  string result;
  string err;
  EvalAppend(env, &result, &err);
  return result;
}

bool EvalString::EvalAppend(Env* env, string* result, string* err) const {
  for (TokenList::const_iterator i = parsed_.begin(); i != parsed_.end(); ++i) {
    if (i->second == RAW) {
      result->append(i->first);
      continue;
    }

    if (!env->AppendVariable(i->first, result, err)) {
      return false;
    }
  }
  return true;
}

void EvalString::AddText(StringPiece text) {
  // Add it to the end of an existing RAW token if possible.
  if (!parsed_.empty() && parsed_.back().second == RAW) {
    parsed_.back().first.append(text.str_, text.len_);
  } else {
    parsed_.push_back(make_pair(text.AsString(), RAW));
  }
}
void EvalString::AddSpecial(StringPiece text) {
  parsed_.push_back(make_pair(text.AsString(), SPECIAL));
}

string EvalString::Serialize() const {
  string result;
  for (TokenList::const_iterator i = parsed_.begin();
       i != parsed_.end(); ++i) {
    result.append("[");
    if (i->second == SPECIAL)
      result.append("$");
    result.append(i->first);
    result.append("]");
  }
  return result;
}
