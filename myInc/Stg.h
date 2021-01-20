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
//
#define Ksz 128         // key-size
#define ks_sz 128       // key-ses-size
#define ss_sz 128       // storage-ses-size

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

void srv_cb(struct evhttp_request *req, SBUF &bfin, SBUF &bfout, void *ctx);

class Stg;
struct Srq {
	/*
	 *   [strict]
	 * 
	 *   Srq-sruct:   xx=xx%xx=xx%....  sep -> % now
	 *    
	 *   ------------------------------------- [K->S]
	 *    
	 *   1 type:      tp=s
	 *   2 operation: op=set | get | (adm/sadm op)          
	 *   3 k-sess:    ks=id|0
	 *   3 s-sess:    ss=id|0
	 *   ....
	 *   
	 *   data -> by op: 
	 *   set: 
	 *       key
	 *   get:
	 *       key
	 *      
	 *   ------------------------------------- [C->S]
	 *   
	 *   
	 * 
	 */
	int stt = 0;
	Stg *bs;
	SBUF *bfin;
	SBUF *bfout;
	// order:
	char tp[8] = { 0 };
	char op[8] = { 0 };
	char ks[ks_sz] = { 0 };
	char ss[ss_sz] = { 0 };
	char key[Ksz] = { 0 };

	Srq(Stg *_bs, SBUF *_bfin, SBUF *_bfout) :
			bs(_bs), bfin(_bfin), bfout(_bfout) {
	}
	
	bool prs() {
		//
		bfin->rnxt(tp, '%', 8);
		//
		if (is(tp, "tp=s")) {
			bfin->rnxt(op, '%', 8);
			//
			if (is(op, "op=...")) {
				int x, y;
				x = bfin->rnxt(ks, '%', ks_sz);
				y = bfin->rnxt(ss, '%', ss_sz);
				//
				if (x <= ks_sz && y <= ss_sz) {
					//
					bfin->rnxt(key, '=', 4);
					//
					if (is(key, "key")) {
						bfin->rnxt(key, 0, Ksz);
					}
					//
					return true;
				}
			}
		}
		// err-parser-case
		bfout->wnxt("S:Srq-parsing-error !");
		//
		return false;
	}
	
	bool is(char *s1, cch *s2, char x = '.') {
		while (*s1 != 0) {
			if (*s1 != *s2 && *s2 != x) return false;
			s1++, s2++;
		}
		if (*s1 == *s2) return true;
		return false;
	}
};

struct Srs {
	
	int stt = 0;
	Stg *bs;
	Srq *srq;
	SBUF *bf;
	Rhnd h;

	Srs(Stg *_bs, Srq *_krq, SBUF *_bf) :
			bs(_bs), srq(_krq), bf(_bf) {
	}
	
};

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
	
	///////////////////////////////////////////////  work space
	
	Rhnd set(cch *id) {
		
		if (STT == PRC) {
			
			Rhnd h = R.new_rec(id);
			
			return h;
		} else {
			//[?]
		}
		
		return Rhnd();
	}
	
	Rhnd get(int grp, ull offs) {
		
		if (STT == PRC) {
			
			Rhnd h = R.get_rec(grp, offs);
			
		} else {
			//[?]
		}
		
		return Rhnd();
	}
	////////////////////////////////////////////////////////////
	
	void err_solver() {
		
	}
	
};

var Stg::EW_stck = varr;
Sew_tp Stg::EW_tp = Sew_tp(0);
Sew_cd Stg::EW_cd = Sew_cd(0);
Sstt_cd Stg::STT = Sstt_cd(0);
bool Stg::EW_PRN = 1;

TMR t;

void srv_cb(struct evhttp_request *req, SBUF &bfin, SBUF &bfout, void *ctx) {
	
	Stg *S = (Stg*) ctx;
	Srq srq(S, &bfin, &bfout);
	Srs srs(S, &srq, &bfout);
	// ---- timer-st
	t.set("S-part-prc");
	//
	bfin.prnt(); /// dbg
	//
	if (srq.prs()) {
		
		if (srq.is(srq.op, "op=set")) {
			//
			t.set("set");
			//
			srs.h = S->set(srq.key);
			//------------------------ tmp
			bfout.wnxt("S:set -> ");
			if (srs.h) bfout.wnxt("[ok] ");
			else bfout.wnxt("[err] ");
			//----------------------------
		} else if (srq.is(srq.op, "op=get")) {
			
			bfout.wnxt("S:op=get");
			
		} else if (srq.is(srq.op, "op=rem")) {
			
			bfout.wnxt("S:op=rem");
			
		} else {
			bfout.wnxt("S:bad [op]");
		}
	}
	// ------------------------ timer-end
	double tm = t.get("S-part-prc");
	bfout.wnxt("\nS:[->S->K] elapsed: ");
	bfout.wnxt(std::to_string(tm).data());
	//------------------------------------
}

}
