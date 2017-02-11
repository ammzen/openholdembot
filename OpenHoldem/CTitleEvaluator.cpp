//******************************************************************************
//
// This file is part of the OpenHoldem project
//    Source code:           https://github.com/OpenHoldem/openholdembot/
//    Forums:                http://www.maxinmontreal.com/forums/index.php
//    Licensed under GPL v3: http://www.gnu.org/licenses/gpl.html
//
//******************************************************************************
//
// Purpose: Evaluating titletext (ttlimits)
//   and the craped c0limitsX
//
//******************************************************************************

#include "stdafx.h"
#include "CTitleEvaluator.h"

#include "CPreferences.h"
#include "CScraper.h"
#include "CTableTitle.h"
#include "..\StringFunctionsDLL\string_functions.h"

//!!!!!
#include "..\CTransform\CTransform.h"

CTableEvaluator *p_title_evaluator = NULL;

CTableEvaluator::CTableEvaluator() {
}

CTableEvaluator::~CTableEvaluator() {
}

void CTableEvaluator::ClearAllDataOncePerHeartbeat() {
  _sblind = kUndefined;
  _bblind = kUndefined;
  _bbet = kUndefined;
  _ante = kUndefined;
  _sb_bb = kUndefined;
  _bb_BB = kUndefined;
  _buyin = kUndefined;
  _limit = kUndefined;
}

void CTableEvaluator::EvaluateTitleText() {
  CString titletext = p_table_title->PreprocessedTitle();
  CString title_format = p_tablemap->GetTMSymbol("ttlimits");
  ProcessTitle(titletext, title_format);
  // Now try alternative ttlimitsX
  for (int i = 0; i < k_max_number_of_titletexts; i++) {
    title_format.Format("ttlimits%d", i);
    ProcessTitle(titletext, title_format);
  }
}

void CTableEvaluator::EvaluateScrapedTitleTexts() {
  EvaluateC0LimitsX("c0limits");
  for (int i = 0; i < k_max_number_of_titletexts; i++) {
    CString c0limitsX;
    c0limitsX.Format("c0limits%d", i);
    EvaluateC0LimitsX(c0limitsX);
  }
}

void CTableEvaluator::EvaluateC0LimitsX(CString c0limitsX) {
  // Lookup s$c0limitsX
  CString title_format = p_tablemap->GetTMSymbol(c0limitsX);
  if (title_format == "") {
    // Not supported by tablemap
    return;
  }
  // Scrape the region r$c0limitsX from the table
  CString scraped_title;
  if (p_scraper->EvaluateRegion(c0limitsX, &scraped_title)) {
    ProcessTitle(scraped_title, title_format);
  }
}

// Former ParseStringBSL()
// For OpenHoldem 7.3.1 we removed the superfluous parameters
// bool *found_handnumber=NULL, bool *found_sblind=NULL, bool *found_bblind=NULL,
// bool *found_bbet=NULL, bool *found_ante=NULL, bool *found_limit=NULL,
// bool *found_sb_bb=NULL, bool *found_bb_BB=NULL
// to get a cleanner interface and cleaner code.
// The other in-out-parameters must be initialized with kUndefined now
// (or with the empty string). This value represents the same information.
bool CTableEvaluator::ProcessTitle(CString title, CString ttlimits_format) {
  // Temp results
  // We only copy them back if title and format match perfectly
  CString new_handnumber = "";
  double new_sblind = kUndefined;
  double new_bblind = kUndefined;
  double new_bbet = kUndefined;
  double new_ante = kUndefined;
  int    new_limit = kUndefined;
  double new_sb_bb = kUndefined;
  double new_bb_BB = kUndefined;
  double new_buyin = kUndefined;
  //
  int     place_in_ttlimits_format = 0, place_in_title = 0;
  CString token = "", skip_str = "", number_type = "";
  double  number = 0.;
  CString temp = "";
  while (place_in_ttlimits_format < ttlimits_format.GetLength() && place_in_title < title.GetLength()) {
    // handnumber
    if (ttlimits_format.Mid(place_in_ttlimits_format, 2) == "^h") {
      place_in_ttlimits_format += 2;
      token = "";
      while (place_in_title < title.GetLength()
        && (isdigit(title[place_in_title]) || (title[place_in_title] == '-'))) {
        if (title[place_in_title] != '-') {
          token += title[place_in_title];
        }
        place_in_title += 1;
      }
      _results_for_openscrape.Append("^h (handnumber)\t= " + token + "\r\n");
      new_handnumber = token.GetString();
    }
    // dummy int
    else if (ttlimits_format.Mid(place_in_ttlimits_format, 2) == "^d") {
      place_in_ttlimits_format += 2;
      token = "";
      while (place_in_title<title.GetLength()
        && (CString(title[place_in_title]).FindOneOf("$0123456789,�km") != -1)) {
        token += (title[place_in_title]);
        place_in_title++;
      }
      _results_for_openscrape.Append("^d (dummy int)\t= " + token + "\r\n");
    }
    // dummy float
    else if (ttlimits_format.Mid(place_in_ttlimits_format, 2) == "^f") {
      place_in_ttlimits_format += 2;
      token = "";
      while (CString(title[place_in_title]).FindOneOf("$0123456789,.�ckm") != -1 && place_in_title<title.GetLength()) {
        token += title[place_in_title];
        place_in_title++;
      }
      _results_for_openscrape.Append("^f (dummy float)\t= " + token + "\r\n");
    }
    // numbers
    else if (ttlimits_format.Mid(place_in_ttlimits_format, 2) == "^s" ||
      ttlimits_format.Mid(place_in_ttlimits_format, 2) == "^b" ||
      ttlimits_format.Mid(place_in_ttlimits_format, 2) == "^B" ||
      ttlimits_format.Mid(place_in_ttlimits_format, 2) == "^a" ||
      ttlimits_format.Mid(place_in_ttlimits_format, 2) == "^y" ||
      ttlimits_format.Mid(place_in_ttlimits_format, 2) == "^v" ||
      ttlimits_format.Mid(place_in_ttlimits_format, 2) == "^V") {
      number_type = ttlimits_format.Mid(place_in_ttlimits_format, 2);
      place_in_ttlimits_format += 2;
      token = "";
      while (CString(title[place_in_title]).FindOneOf("$0123456789,.�ckm") != -1 && place_in_title<title.GetLength()) {
        token += title[place_in_title];
        place_in_title++;
      }
      CTransform t;
      number = t.StringToMoney(token);
      temp = Number2CString(number);
      if (number_type == "^s") {
        _results_for_openscrape.Append("^s (sblind)\t= " + temp + "\r\n");
        new_sblind = number;
      }
      else if (number_type == "^b") {
        _results_for_openscrape.Append("^b (bblind)\t= " + temp + "\r\n");
        new_bblind = number;
      }
      else if (number_type == "^B") {
        _results_for_openscrape.Append("^B (bbet)\t= " + temp + "\r\n");
        new_bbet = number;
      }
      else if (number_type == "^a") {
        _results_for_openscrape.Append("^a (ante)\t= " + temp + "\r\n");
        new_ante = number;
      }
      else if (number_type == "^v") {
        _results_for_openscrape.Append("^v (sb_bb)\t= " + temp + "\r\n");
        new_sb_bb = number;
      }
      else if (number_type == "^V") {
        _results_for_openscrape.Append("^V (bb_BB)\t= " + temp + "\r\n");
        new_bb_BB = number;
      }
      else if (number_type == "^y") {
        _results_for_openscrape.Append("^y (buyin)\t= " + temp + "\r\n");
        new_buyin = number;
      }
    }
    // Skip all chars up to string following the ^*
    else if (ttlimits_format.Mid(place_in_ttlimits_format, 2) == "^*") {
      place_in_ttlimits_format += 2;
      skip_str = "";
      token = "";
      // Get the string that we want to match up to
      while (ttlimits_format[place_in_ttlimits_format] != '^' && place_in_ttlimits_format < ttlimits_format.GetLength()) {
        skip_str += ttlimits_format[place_in_ttlimits_format];
        place_in_ttlimits_format += 1;
      }
      while (title.Mid(place_in_title, skip_str.GetLength()) != skip_str &&
        place_in_title < title.GetLength()) {
        token += title[place_in_title];
        place_in_title += 1;
      }
      if (title.Mid(place_in_title, skip_str.GetLength()) == skip_str) {
        token.Append(title.Mid(place_in_title, skip_str.GetLength()));
        place_in_title += skip_str.GetLength();
      }
      _results_for_openscrape.Append("^* (skip string)\t= '" + token + "'\r\n");
    }
    // Roman numerals
    else if (ttlimits_format.Mid(place_in_ttlimits_format, 2) == "^R") {
      place_in_ttlimits_format += 2;
      token = "";
      // Get roman numeral
      while ((tolower(title[place_in_title]) == 'i'
        || tolower(title[place_in_title]) == 'v'
        || tolower(title[place_in_title]) == 'x'
        || tolower(title[place_in_title]) == 'l'
        || tolower(title[place_in_title]) == 'c')
        && place_in_title < title.GetLength()) {
        token += (unsigned char)tolower(title[place_in_title]);
        place_in_title += 1;
      }
      // This is commented out for now, because, strangely, WH does not provide a
      // place to report this value to the user
      // convert roman numeral to number
      //if (token == "i") {
      //      level = 1;
      //}
      //if (token == "ii") {
      //      level = 2;
      //}
      _results_for_openscrape.Append("^R (roman numeral)\t= " + token + "\r\n");
    }
    // Limit, no-limit, pot limit identifier
    else if (ttlimits_format.Mid(place_in_ttlimits_format, 2) == "^L") {
      place_in_ttlimits_format += 2;
      if (title.Mid(place_in_title, 8).MakeLower() == "no limit"
        || title.Mid(place_in_title, 8).MakeLower() == "no-limit") {
        _results_for_openscrape.Append("^L (limit type)\t= " + title.Mid(place_in_title, 8) + "\r\n");
        new_limit = kGametypeNL;
        place_in_title += 8;
      }
      else if (title.Mid(place_in_title, 7).MakeLower() == "nolimit") {
        _results_for_openscrape.Append("^L (limit type)\t= " + title.Mid(place_in_title, 7) + "\r\n");
        new_limit = kGametypeNL;
        place_in_title += 7;
      }
      else if (title.Mid(place_in_title, 2).MakeLower() == "nl") {
        _results_for_openscrape.Append("^L (limit type)\t= " + title.Mid(place_in_title, 2) + "\r\n");
        new_limit = kGametypeNL;
        place_in_title += 2;
      }
      else if (title.Mid(place_in_title, 9).MakeLower() == "pot limit"
        || title.Mid(place_in_title, 9).MakeLower() == "pot-limit") {
        _results_for_openscrape.Append("^L (limit type)\t= " + title.Mid(place_in_title, 9) + "\r\n");
        new_limit = kGametypePL;
        place_in_title += 9;
      }
      else if (title.Mid(place_in_title, 8).MakeLower() == "potlimit") {
        _results_for_openscrape.Append("^L (limit type)\t= " + title.Mid(place_in_title, 8) + "\r\n");
        new_limit = kGametypePL;
        place_in_title += 8;
      }
      else if (title.Mid(place_in_title, 2).MakeLower() == "pl") {
        _results_for_openscrape.Append("^L (limit type)\t= " + title.Mid(place_in_title, 2) + "\r\n");
        new_limit = kGametypePL;
        place_in_title += 2;
      }
      else if (title.Mid(place_in_title, 11).MakeLower() == "fixed limit"
        || title.Mid(place_in_title, 11).MakeLower() == "fixed-limit") {
        _results_for_openscrape.Append("^L (limit type)\t= " + title.Mid(place_in_title, 11) + "\r\n");
        new_limit = kGametypeFL;
        place_in_title += 11;
      }
      else if (title.Mid(place_in_title, 10).MakeLower() == "fixedlimit") {
        _results_for_openscrape.Append("^L (limit type)\t= " + title.Mid(place_in_title, 10) + "\r\n");
        new_limit = kGametypeFL;
        place_in_title += 10;
      }
      else if (title.Mid(place_in_title, 2).MakeLower() == "fl") {
        _results_for_openscrape.Append("^L (limit type)\t= " + title.Mid(place_in_title, 2) + "\r\n");
        new_limit = kGametypeFL;
        place_in_title += 2;
      }
      else if (title.Mid(place_in_title, 5).MakeLower() == "limit") {
        _results_for_openscrape.Append("^L (limit type)\t= " + title.Mid(place_in_title, 5) + "\r\n");
        new_limit = kGametypeFL;
        place_in_title += 5;
      }
    }
    // All other exact matches
    else if (title[place_in_title] == ttlimits_format[place_in_ttlimits_format]) {
      _results_for_openscrape.Append("exact match\t= '");
      while (title[place_in_title] == ttlimits_format[place_in_ttlimits_format] && place_in_title<title.GetLength() && place_in_ttlimits_format<ttlimits_format.GetLength()) {
        _results_for_openscrape += (title[place_in_title]);
        place_in_title += 1;
        place_in_ttlimits_format += 1;
      }
      _results_for_openscrape.Append("'\r\n");
    }
    else {
      // Match failed
      // Don't write any temporar5y values back.
      // Just quit
      // log!!!
      return false;
    }
  }
  // Perfect match found.
  // Write temporary results back.
  // Only if the lnown value is "undefined",
  // without any worries about the new value
  if (_handnumber == "") {
    _handnumber = new_handnumber;
  }
  if (_sblind == kUndefined) {
    _sblind = new_sblind;
  }
  if (_bblind == kUndefined) {
    _bblind = new_bblind;
  }
  if (_bbet == kUndefined) {
    _bbet = new_bbet;
  }
  if (_ante == kUndefined) {
    _ante = new_ante;
  }
  if (_limit == kUndefined) {
    _limit = new_limit;
  }
  if (_sb_bb == kUndefined) {
    _sb_bb = new_sb_bb;
  }
  if (_bb_BB == kUndefined) {
    _bb_BB = new_bb_BB;
  }
  if (_buyin == kUndefined) {
    _buyin = new_buyin;
  }
#ifdef OPENHOLDEM_PROGRAM
  write_log(preferences.debug_scraper(),
    "[CTransform] parsed title string\n");
  write_log(preferences.debug_scraper(),
    "[CTransform] title = %s, format = %s\n",
    title, ttlimits_format);
  write_log(preferences.debug_scraper(),
    "[CTransform] handnumber = %s, sblind = %f, bblind = %f\n",
    _handnumber, _sblind, _bblind);
  write_log(preferences.debug_scraper(),
    "[CTransform] bbet = %f, ante = %f, limit = %d, sb_bb = %f, bb_BB = %f\n",
    _bbet, _ante, _limit, _sb_bb, _bb_BB);
  write_log(preferences.debug_scraper(),
    "[CTransform] buyin = %d\n",
    _buyin);
#endif
  return true;
}
