/*
 * 
 *            Stg - infrastructure
 *            start: 21.12.2020
 *            
 *            
 * 
 */
#include "../../cli_dev/myInc/cli.h"
#include "../../srv_dev/myInc/srv.h"
#include "../../VAR_dev/myInc/var.h"
#include "../../rec_dev/myInc/rec.h"
#include "../../F_dev/X/F.h"

namespace NStg {
   
using namespace NRec;

//types:
typedef const char cch;
typedef unsigned long long ull;

enum Sew_tp {
	FATAL = 1, USER, INNER, WRN
};
enum Sew_cd {
	UNEXPECTED = 1, BAD_PARAM, BAD_USE, REC_FAIL
};
enum Sstt_cd {
	INIT = 1, // init-stage
	PRC,	  // ok-state
	EXIT,	  // fail-exit
};

class SExcp: public std::exception {
	
	cch *exc = 0;

public:

	mutable Sew_tp ERR_tp = Sew_tp(0);
	mutable Sew_cd ERR_cd = Sew_cd(0);
	mutable Sstt_cd STT = Sstt_cd(0);
	//
	SExcp() {
	}
	
	SExcp(cch *_exc) noexcept {
		exc = _exc;
	}
	
	SExcp(Sew_tp _ERR_tp, Sew_cd _ERR_cd, Sstt_cd _STT) noexcept {
		ERR_tp = _ERR_tp;
		ERR_cd = _ERR_cd;
		STT = _STT;
	}
	
	SExcp(cch *_exc, Sew_tp _ERR_tp, Sew_cd _ERR_cd, Sstt_cd _STT) {
		exc = _exc;
		ERR_tp = _ERR_tp;
		ERR_cd = _ERR_cd;
		STT = _STT;
	}
	
	~SExcp() noexcept {
	}
	
	const char* what() const noexcept {
		return exc;
	}
	
};

str* srv_cb(str &sin, void *ctx);

class Stg {
	
	static var EW_stck;
	static Sew_tp EW_tp;
	static Sew_cd EW_cd;
	static Sstt_cd STT;
	static bool EW_PRN;

	CLI cli;
	SRV srv;
	Rec R;

public:

	Stg(cch *rec_fld = "rec") :
			R(rec_fld) {
		STT = INIT;
		//
	}
	~Stg() {
	}
	
	//------------------ if:
	
	static void EW_set(cch *ctx, Sew_tp tp, Sew_cd cd, var addt, var _at) {
		
		EW_tp = tp;
		EW_cd = cd;
		EW_stck.push_back(var( { ctx, int(tp), int(cd), int(STT), addt, _at }));
		//
		if (EW_PRN || FATAL) EW_stck[EW_stck.length() - 1].prnt();
		//
		if (tp == FATAL) throw(SExcp(tp, cd, STT));
		
	}
	
	void init(bool no_srv = 0) {
		/* 
		 *   check:
		 *   
		 *        inner-status: - own-err , rec-stt , ... 
		 *                      
		 *        config:       - syntax/param-check
		 *                      - servers-reachability
		 *                      - ....
		 *                      - min-requirements
		 *        start:
		 *                      - K-server start
		 *                      - ....        
		 *        process ...  
		 *                    
		 * 
		 */
		try {
			// ----------------------------------- inner-status: 
			std::cout << "inner-status: ...";
			// ---------- own:
			// ....    
			// ---------- rec:
			R.init();
			if (R.STT != NRec::PRC) {
				Stg::EW_set("[Key:err:F]", FATAL, REC_FAIL, //
						"REC_FAIL: init-stage '", { at });
			}
			//tmp
			R.destroy();
			
			std::cout << "ok" << std::endl;
			// ------------------------------------ config:
			std::cout << "config: ...";
			//--- set-work state
			
			STT = PRC;
			std::cout << "ok" << std::endl;
			// ------------------------------------ start:	
			std::cout << "start: ..." << std::endl;
			
			srv.set("-a 192.168.100.6 -p 8888"); //--prr
			srv.set_ext_cb(srv_cb, this);
			srv.listen();  
						
		} catch (SExcp &ex) {
			err_solver();
		}
	}
	
	void err_solver() {
		
	}
	
};

var Stg::EW_stck = varr;
Sew_tp Stg::EW_tp = Sew_tp(0);
Sew_cd Stg::EW_cd = Sew_cd(0);
Sstt_cd Stg::STT = Sstt_cd(0);
bool Stg::EW_PRN = 1;

str* srv_cb(str &sin, void *ctx) {
	
	//double tm = 0;
	//ull tot = 0;
	
	static str s = "---AAAAAAAAAA";
	var x, y, z;
	//-------------------   будет спец-парсер
//	x = var(sin).split('%');	////   var-error if sin empty !!!!! 
//	y = var(x[0]).split('=');
//	z = var(x[1]).split('=');
	var(sin).prnt();
	//-------------------
	//
	//
	return &s;
}

}
