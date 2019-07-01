
"""
/////////////////////////////////////////////////////////////////////////////
  Author      :     P.S. Mandrik, IHEP
  Date        :     11/09/16
  Last Update :     13/10/16
  Version     :     0.5.5
/////////////////////////////////////////////////////////////////////////////

  Python module TemplateMaster.py provide function 
    def parce_template(template, dic)
  thats parce template(=string ) and transform due to rules in dic(=dictionary)
  template transformation patterns:

    {% var_name %} -> dic[var_name] # just a variable

    {% for item in list_name %}  -> dic[list_name][0]  # expand for loop
      {% item %}                    dic[list_name][1]
    {% endfor %}                    dic[list_name][2]
                                          ...

    {% if var_name %}            -> True # if dic[var_name] == True  # expand if statemant
      True
    {% else %}
      False
    {% endif %}


    {% func nice_format [params, ... ] %}       -> dic["nice_format"](text[, params, ... ])
      text 
    {% endfunc %}

  see function examples() for example

  RESERVED NAMES :
    {%, %}
    for, endfor
    if, else, endif
    func, endfunc

  TODO:
    - complex if expression
    - expand functions with brackets and parameters in them?
    - complex variables expressions

  EXTRA FUNCTIONS :
  In addition specially for func key worlds some functions added:

    0) add_TemplateMaster_dictionary(dic) - add to dic following functions

    1) tabulate(text, split_symbol=None, n_spaces=3)   - split lines in split_symbol into columns,
                                                         format they width and return new nice text

    2) textwrap(text, width=100)                       - wrap text into width and split into multilines 

    3) insert_comments(text, symbol="#", n_spaces = 3) - insert symbol before at the beginning every new line

/////////////////////////////////////////////////////////////////////////////
  Changelog : 

    13/10/16  - 0.5.5
                - add func and endfunc statement to aplly function to part of texts
                inside the templates, so now:
                {% func nice_format p1 p2 %} text {% endfunc %} -> nice_format(text, p1, p2)

                - fix stability of extra spaces in matching functions

                - new functions:
                    tabulate(text, split_symbol=None, n_spaces=3)
                    textwrap(text, width=100)
                    insert_comments(text, symbol="#", n_spaces = 3)
                    add_TemplateMaster_dictionary(dic):

    17/09/16  - 0.5.1
                if and for statemant can expand attribute call chain as a variables before
                so now possible to write {% if var.attr_call.next_call.and_last_one %}
                and {% for a in var.attr_call.next_call.and_last_one %}, v.0.5.1
    
    11/09/16  - 0.5.0
                day of creation, version
"""

import re
import copy

####################################### parce_template support functions ###################

def is_string(match, string):
  groups = match.groups()
  if not len(groups) or groups[0] != string : return False
  return True

def find_groups(match, regexp):
  match_groups = match.groups()
  if not len(match_groups) : return None
  groups = re.findall(regexp, match_groups[0] )
  if not len(groups) or not len(groups[0]) : return None
  return groups

def is_for(match):          return find_groups(match, "for[ ]+([^\s]+)[ ]+in[ ]+([^\s]+)")
def is_if(match):           return find_groups(match, "if[ ]+([^\s]+)")
def is_func(match):         return find_groups(match, "func[ ]+([^\s]+)[ ]*(.*)")
#def is_attr(match):         return find_groups(match, "(.+)\.(.+)")
def is_attr_old(match): 
  match_groups = match.groups()
  if not len(match_groups) : return None
  groups = match_groups[0].split(".")
  if len(groups) < 2 : return None
  #print match_groups[0], groups
  return groups

def is_endfor(match): return is_string(match, "endfor")
def is_else(match)  : return is_string(match, "else"  )
def is_endif(match) : return is_string(match, "endif" )
def is_endfunc(match) : return is_string(match, "endfunc" )

def is_var(match):
  string = match.groups()[0]
  string.split(" ")
  if len(string.split(" ")) == 1: return string
  return None

def print_mathcs(matchs_list):
  print [m.groups()[0] for m in matchs_list]

def get_subtext(subtemplates, matchs_list, start, end, skip_last = False):
  subtext = ""
  for i in xrange(start, end):
    subtext += subtemplates[i]
    if skip_last and i == end - 1 : continue
    else : subtext += "{% " + matchs_list[i].groups()[0] + " %}"
  return subtext

def remove_newline(sub_text):
  for i, char in enumerate(sub_text) :
    if char not in " \n": break
    if char == "\n" : 
      return sub_text[i+1:]
  return sub_text

def remove_spaces_before(string):
  for i in reversed(xrange(len(string))):
    char = string[i]
    if char not in "\n " : break
    if char == "\n": return string[:i+1]
  return string

def find_pairs(f_start, f_end, matchs_list):
  pairs = []
  for i_start, match_start in enumerate(matchs_list):
    # find start
    start_groups = f_start(match_start)
    if not start_groups : continue
    # find end
    brack_index = 1
    i_end = 0
    for i, match_end in enumerate(matchs_list):
      if i <= i_start : continue
      if f_start(match_end) : brack_index += 1
      if f_end(match_end) : 
        brack_index -= 1
        if not brack_index : 
          i_end = i
          break
    if not i_end : 
      print "WARNING : find_pairs() : no end for this start : ", match_start.groups()[0]
      continue

    match_end = matchs_list[i_end]
    pairs += [ [match_start, match_end] ]
  return pairs

def find_triples(f_start, f_middle, f_end, matchs_list):
  triples = []
  for i_start, match_start in enumerate(matchs_list):
    # find start
    start_groups = f_start(match_start)
    if not start_groups : continue
    # find end
    brack_index = 1
    i_end = 0
    for i, match_end in enumerate(matchs_list):
      if i <= i_start : continue
      if f_start(match_end) : brack_index += 1
      if f_end(match_end) : 
        brack_index -= 1
        if not brack_index : 
          i_end      = i
          break
    if not i_end : 
      print "WARNING : find_pairs() : no end for this start : ", match_start.groups()[0]
      continue
    match_end = matchs_list[i_end]

    # find middle, optional
    brack_index = 1
    i_middle = 0
    for i, match_middle in enumerate(matchs_list):
      if i <= i_start or i >= i_end: continue
      if f_start(match_middle)  : brack_index += 1
      if f_end(match_middle)    : brack_index -= 1
      if f_middle(match_middle) :
        if brack_index == 1: 
          i_middle = i
          break

    if i_middle : match_middle = matchs_list[i_middle]
    else        : match_middle = None
    triples += [ [match_start, match_middle, match_end] ]
  return triples

# expand method key chain
NO_KEY_CODE = "_____CANT_EXPAND_WALUE_FROM_DICTIONARY_CODE_|;_;|"
def expand_key(key, dic, call = True):
  # if it just a key
  if key in dic: return dic[key]

  # maybe it is a chain of attr calls
  groups = key.split(".")
  if len(groups) < 2 : return NO_KEY_CODE

  var_key = groups[0]
  method_keys = groups[1:]

  if var_key not in dic: return NO_KEY_CODE
  var = dic[var_key]

  for method_key in method_keys:
    if not hasattr(var, method_key) : 
      print "WARNING : parce_template() : class " + var_key + " hasn't attribute with name " + method_key
      return NO_KEY_CODE

    attr = getattr(var, method_key)
    if callable(attr) and call: var = attr()
    else              : var = attr
  return var

####################################### parce_template ###################
def parce_template(template, dic):
  ntemplate = copy.copy(template)
  ndic      = copy.copy(dic)

  #ntemplate = re.sub("\n", "{*}", ntemplate)

  # find brackets {% %}
  matchs = re.finditer("{% +(.+?) +%}", ntemplate )

  # divide templates
  nsubtemplates = []
  prev = 0
  matchs_list = []
  for match in matchs:
    matchs_list += [ match ]
    subtemplate = ntemplate[prev : match.start()]
    prev = match.end()
    nsubtemplates += [subtemplate]
  if len(matchs_list): nsubtemplates += [ntemplate [matchs_list[-1].end() : ]]
  else : return ntemplate

  # work with for
  # find for loops and endfor pairs
  for_pairs = find_pairs(is_for, is_endfor, matchs_list)

  # expand hight level for loop, and recursevly call this function for text inside loop
  for pair in for_pairs :
    for_start = pair[0]
    for_end   = pair[1]

    if for_start not in matchs_list or for_end not in matchs_list : continue

    start_index = matchs_list.index(for_start)
    end_index   = matchs_list.index(for_end  )

    for_start_text_pos = for_start.end()
    for_end_text_pos   = for_end.start()

    #sub_text2    = ntemplate[for_start_text_pos : for_end_text_pos]
    sub_text = get_subtext(nsubtemplates, matchs_list, start_index+1, end_index+1, True)

    # remove \n after for loop
    # check, if them exist
    sub_text = remove_newline(sub_text)

    # remove spaces before endfor if them exist
    sub_text = remove_spaces_before(sub_text)
    
    for_groups = is_for(for_start)
    list_key = for_groups[0][1]
    var_key  = for_groups[0][0]

    list = expand_key(list_key, dic)
    if list == NO_KEY_CODE : continue

    nsub_text = ""
    for var in list : 
      ndic = copy.copy(dic)
      ndic[var_key] = var
      nsub_text += parce_template(sub_text, ndic)

    # remove spaces before for loop if it is at new line
    nsubtemplates[start_index] = remove_spaces_before(nsubtemplates[start_index])

    new_subtemplates = nsubtemplates[:start_index+1]
    #new_subtemplates[start_index] += nsub_text + remove_newline(nsubtemplates[end_index+1])
    new_subtemplates[start_index] += nsub_text + nsubtemplates[end_index+1]
    if len(nsubtemplates) > end_index+1 : new_subtemplates += nsubtemplates[end_index+2:]
    nsubtemplates = new_subtemplates
    matchs_list   = matchs_list[:start_index] + matchs_list[end_index+1:]

  # work with if else endif
  # find triples
  if_pairs = find_triples(is_if, is_else, is_endif, matchs_list)

  # expand hight level if, and recursevly call this function for text inside ifs
  for pair in if_pairs :
    if_start  = pair[0]
    if_middle = pair[1]
    if_end    = pair[2]

    if if_start not in matchs_list or if_end not in matchs_list : continue
    if if_middle and if_middle not in matchs_list : continue

    start_index  = matchs_list.index(if_start)
    end_index    = matchs_list.index(if_end  )

    middle_index = -1
    if if_middle :
      if if_middle not in matchs_list : continue
      middle_index = matchs_list.index(if_middle)

    # check if variable
    if_groups = is_if(if_start) 
    var_key   = if_groups[0]

    # expand it, if a chain of methods:
    val = expand_key(var_key, dic)
    if val == NO_KEY_CODE  : continue
    
    sub_text = ""
    if val     and     if_middle : # from if to else
      sub_text = get_subtext(nsubtemplates, matchs_list, start_index+1, middle_index+1, True)
    if val     and not if_middle : # from if to endif
      sub_text = get_subtext(nsubtemplates, matchs_list, start_index+1, end_index+1, True)
    if not val and     if_middle : # go to else if exist
      sub_text = get_subtext(nsubtemplates, matchs_list, middle_index+1, end_index+1, True)

    # remove \n after operators, check, if them exist
    sub_text = remove_newline(sub_text)

    # remove spaces before endfor if them exist
    sub_text = remove_spaces_before(sub_text)

    nsub_text = parce_template(sub_text, dic)

    # remove spaces before if when it is at new line
    nsubtemplates[start_index] = remove_spaces_before(nsubtemplates[start_index])

    new_subtemplates = nsubtemplates[:start_index+1]
    new_subtemplates[start_index] += nsub_text + remove_newline(nsubtemplates[end_index+1])
    #new_subtemplates[start_index] += nsub_text + nsubtemplates[end_index+1]
    if len(nsubtemplates) > end_index+1 : new_subtemplates += nsubtemplates[end_index+2:]
    nsubtemplates = new_subtemplates
    matchs_list   = matchs_list[:start_index] + matchs_list[end_index+1:]

  """
  # work with variables method cals
  for i, match in zip(reversed(xrange(len(matchs_list))), reversed(matchs_list)):
    groups = is_attr( match )
    if not groups : continue

    var_key = groups[0]
    method_keys = groups[1:]

    if var_key not in dic: continue
    var = dic[var_key]

    for method_key in method_keys:
      #print var, method_key, method_keys
      if not hasattr(var, method_key) : 
        print "WARNING : parce_template() : class " + var_key + " hasn't attribute with name " + method_key
        continue

      val = getattr(var, method_key)
      if callable(val) : var = val()
      else             : var = val
    sub_text = str( var )

    new_subtemplates = nsubtemplates[:i+1]
    new_subtemplates[i] += sub_text + nsubtemplates[i+1]
    if len(nsubtemplates) > i+1 : new_subtemplates += nsubtemplates[i+2:]
    nsubtemplates = new_subtemplates
    matchs_list = matchs_list[:i] + matchs_list[i+1:]
  """

  # work with func
  func_pairs = find_pairs(is_func, is_endfunc, matchs_list)
  for pair in func_pairs  :
    func_start = pair[0]
    func_end   = pair[1]

    if func_start not in matchs_list or func_end not in matchs_list : continue

    start_index = matchs_list.index(func_start)
    end_index   = matchs_list.index(func_end  )

    func_start_text_pos = func_start.end()
    func_end_text_pos   = func_end.start()

    sub_text = get_subtext(nsubtemplates, matchs_list, start_index+1, end_index+1, True)
    sub_text = remove_newline(sub_text)
    sub_text = remove_spaces_before(sub_text)
    
    func_groups = is_func(func_start)
    func_key = func_groups[0][0]
    params_keys = func_groups[0][1]
    params_keys = params_keys.split(" ")

    func = expand_key(func_key, dic, False)
    if func == NO_KEY_CODE : continue

    if not callable(func) :
      print "WARNING: parce_template() : func '" + func_key + "' should be callable"
      continue

    params = []
    if len(params_keys) :
      for params_key in params_keys:
        param = expand_key(params_key, dic)
        params += [ param ]
        if param == NO_KEY_CODE : 
          params = []
          break
      
    if params : nsub_text = func( parce_template(sub_text, dic), *params )
    else      : nsub_text = func( parce_template(sub_text, dic) )

    # remove spaces before for loop if it is at new line
    nsubtemplates[start_index] = remove_spaces_before(nsubtemplates[start_index])

    new_subtemplates = nsubtemplates[:start_index+1]
    new_subtemplates[start_index] += nsub_text + nsubtemplates[end_index+1]
    if len(nsubtemplates) > end_index+1 : new_subtemplates += nsubtemplates[end_index+2:]
    nsubtemplates = new_subtemplates
    matchs_list   = matchs_list[:start_index] + matchs_list[end_index+1:]
    
  # work with variables
  for i, match in zip(reversed(xrange(len(matchs_list))), reversed(matchs_list)):
    var_key = is_var( match )
    if not var_key : continue

    var = expand_key(var_key, dic)
    if var == NO_KEY_CODE : continue
    
    new_subtemplates = nsubtemplates[:i+1]
    new_subtemplates[i] += str(var) + nsubtemplates[i+1]
    if len(nsubtemplates) > i+1 : new_subtemplates += nsubtemplates[i+2:]
    nsubtemplates = new_subtemplates
    matchs_list = matchs_list[:i] + matchs_list[i+1:]

  ntemplate = get_subtext(nsubtemplates, matchs_list, 0, len(nsubtemplates)-1) + nsubtemplates[-1]

  return ntemplate

####################################### usefull functions ###################
def tabulate(text, split_symbol=None, n_spaces=3):
  lines = text.split("\n")
  for i, line in enumerate(lines):
    if split_symbol : lines[i] = line.split(split_symbol)
    else : lines[i] = line.split()
  
  colums_N = len(max(lines, key=len))
  colums_width = [ 0 for i in xrange(colums_N)]
  for line in lines:
    for i, column in enumerate(line):
      colums_width[i] = max(colums_width[i], len(column))

  out_lines = ""
  for line in lines:
    for width, column in zip(colums_width, line):
      out_lines += column + " " * (width - len(column) + n_spaces)
    out_lines += "\n"
  return out_lines

def textwrap(text, width=100):
  import textwrap
  return textwrap.fill(text, width)

def insert_comments(text, symbol="#", n_spaces = 3):
  lines = text.split("\n")
  for i, line in enumerate(lines):
    lines[i] = symbol + " " * n_spaces + line
  return "\n".join(lines)

def add_TemplateMaster_dictionary(dic):
  deff_dic = {}
  deff_dic["tabulate"] = tabulate
  deff_dic["textwrap"] = textwrap
  deff_dic["insert_comments"] = insert_comments
  dic.update(deff_dic)

def examples():
  ####################################### example  # 0 ###################
  template_A = """
{% var_1 %} = {% var_2 %}
{% for A in list_A %}
  {% A %} = {% for B in list_B %}{% B %}-{% endfor %}0
{% endfor %}
{% if var_1 %}
  True
{% endif %}
"""

  answer_A = """
0 = hello_worlds
  Var_A = 10-11-12-13-14-0
  Var_B = 10-11-12-13-14-0

"""

  dic = {"var_1" : 0, "var_2" : "hello_worlds", "list_A" : ["Var_A", "Var_B"], "list_B" : [i for i in xrange(10, 15)]}
  answer = parce_template(template_A, dic)
  print template_A, " -> ", answer, "answer_A == answer : ", answer_A == answer

  #######################################  example # 1 ###################
  class A:
    variable = "qwerty"
    def get_variable(self):
      return self.variable + " from get_variable() "
    def get_self(self):
      return self

  template_B = """
    {% var.variable %}
    {% var.get_variable %}
    {% var.get_self.get_variable %} # 2
"""
  answer_B = """
    qwerty
    qwerty from get_variable() 
    qwerty from get_variable()  # 2
"""

  dic = {"var" : A() }
  answer = parce_template(template_B, dic)
  print template_B, " -> ", answer, "answer_B == answer : ", answer_B == answer

  #######################################  example # 2 ###################
  template_C = """
    {% for text   in  texts %}
      {% text %}
      my_func-> {% func B.my_func %}{% text %}{% endfunc %}
      my_func_2-> {% func B.my_func_2 B.get_params_start B.get_params_end %}{% text %}{% endfunc %}
    {% endfor %}
"""

  class my_class():
    def __init__(self):
      self.i = -1;

    def my_func(self, text):
      return text.upper()

    def my_func_2(self, text, start, end):
      print text, start, end
      return text[start : end].upper()

    def get_params_start(self):
      self.i += 1
      return self.i

    def get_params_end(self):
      return self.i + 2

  texts = ["qwerty", "asdfgh", "zxcvbv"]
  dic = {"texts" : texts, "B" : my_class()}
  answer = parce_template(template_C, dic)
  print template_C, " -> ", answer
  
if __name__ == "__main__": examples()


















