//-----------------------------------------------------------------------------
// name: Faust.cpp
// desc: Faust ChucK chugin
//
// authors: Ge Wang (ge@ccrma.stanford.edu)
//          Romain Michon (rmichon@ccrma.stanford.edu)
// date: Spring 2016
//
// NOTE: be mindful of chuck/chugin compilation, particularly on OSX
//       compiled for 10.5 chuck may not work well with 10.10 chugin!
//-----------------------------------------------------------------------------
// this should align with the correct versions of these ChucK files
#include "chuck_dl.h"
#include "chuck_def.h"

// general includes
#include <stdio.h>
#include <limits.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
using namespace std;

// faust include
#include "faust/dsp/llvm-dsp.h"
#include "faust/gui/UI.h"
#include "faust/gui/PathBuilder.h"




// declaration of chugin constructor
CK_DLL_CTOR(faust_ctor);
// declaration of chugin desctructor
CK_DLL_DTOR(faust_dtor);
// for Chugins extending UGen, this is mono synthesis function for 1 sample
CK_DLL_TICK(faust_tick);

// example of getter/setter
CK_DLL_MFUN(faust_eval);
CK_DLL_MFUN(faust_compile);
CK_DLL_MFUN(faust_v_set);
CK_DLL_MFUN(faust_v_get);
CK_DLL_MFUN(faust_ok);
CK_DLL_MFUN(faust_error);
CK_DLL_MFUN(faust_code);
CK_DLL_MFUN(faust_test);

// this is a special offset reserved for Chugin internal data
t_CKINT faust_data_offset = 0;

#ifndef FAUSTFLOAT
  #define FAUSTFLOAT float
#endif




//-----------------------------------------------------------------------------
// name: class FauckUI
// desc: Faust ChucK UI -> map of complete hierarchical path and zones
//-----------------------------------------------------------------------------
class FauckUI : public UI, public PathBuilder
{
protected:
    // name to pointer map
    std::map<std::string, FAUSTFLOAT*> fZoneMap;
    
    // insert into map
    void insertMap( std::string label, FAUSTFLOAT * zone )
    {
        // map
        fZoneMap[label] = zone;
    }
    
public:
    // constructor
    FauckUI() { }
    // destructor
    virtual ~FauckUI() { }
    
    // -- widget's layouts
    void openTabBox(const char* label)
    {
        fControlsLevel.push_back(label);
    }
    void openHorizontalBox(const char* label)
    {
        fControlsLevel.push_back(label);
    }
    void openVerticalBox(const char* label)
    {
        fControlsLevel.push_back(label);
    }
    void closeBox()
    {
        fControlsLevel.pop_back();
    }
    
    // -- active widgets
    void addButton(const char* label, FAUSTFLOAT* zone)
    {
        insertMap(buildPath(label), zone);
    }
    void addCheckButton(const char* label, FAUSTFLOAT* zone)
    {
        insertMap(buildPath(label), zone);
    }
    void addVerticalSlider(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT fmin, FAUSTFLOAT fmax, FAUSTFLOAT step)
    {
        insertMap(buildPath(label), zone);
    }
    void addHorizontalSlider(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT fmin, FAUSTFLOAT fmax, FAUSTFLOAT step)
    {
        insertMap(buildPath(label), zone);
    }
    void addNumEntry(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT fmin, FAUSTFLOAT fmax, FAUSTFLOAT step)
    {
        insertMap(buildPath(label), zone);
    }
    
    // -- passive widgets
    void addHorizontalBargraph(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT fmin, FAUSTFLOAT fmax)
    {
        insertMap(buildPath(label), zone);
    }
    void addVerticalBargraph(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT fmin, FAUSTFLOAT fmax)
    {
        insertMap(buildPath(label), zone);
    }
    
    // -- metadata declarations
    void declare(FAUSTFLOAT* zone, const char* key, const char* val)
    {}
    
    // set/get
    void setValue( const std::string& path, FAUSTFLOAT value )
    {
        // TODO: should check if path valid?
        
        // set it!
        *fZoneMap[path] = value;
    }
    
    float getValue(const std::string& path)
    {
        return *fZoneMap[path];
    }
    
    // map access
    std::map<std::string, FAUSTFLOAT*>& getMap() { return fZoneMap; }
    // get map size
    int getParamsCount() { return fZoneMap.size(); }
    // get param path
    std::string getParamPath(int index)
    {
        std::map<std::string, FAUSTFLOAT*>::iterator it = fZoneMap.begin();
        while (index-- > 0 && it++ != fZoneMap.end()) {}
        return (*it).first;
    }
};




//-----------------------------------------------------------------------------
// name: class Faust
// desc: class definition of internal chugin data
//-----------------------------------------------------------------------------
class Faust
{
public:
    // constructor
    Faust( t_CKFLOAT fs)
    {
        // sample rate
        m_srate = fs;
        // clear
        m_factory = NULL;
        m_dsp = NULL;
        m_ui = NULL;
        // zero
        m_input = NULL;
        m_output = NULL;
    }
    
    // destructor
    ~Faust()
    {
        // clear
        clear();
    }
    
    // clear
    void clear()
    {
        if( m_input != NULL ) SAFE_DELETE_ARRAY(m_input[0]);
        if( m_output != NULL ) SAFE_DELETE_ARRAY(m_output[0]);
        SAFE_DELETE_ARRAY(m_input);
        SAFE_DELETE_ARRAY(m_output);
        
        // clean up, possibly
        if( m_factory != NULL )
        {
            deleteDSPFactory( m_factory );
            m_factory = NULL;
        }
        SAFE_DELETE(m_dsp);
        SAFE_DELETE(m_ui);
    }
    
    // eval
    bool eval( const string & code )
    {
        // clean up
        clear();
        
        // arguments
        const int argc = 0;
        const char ** argv = NULL;
        // optimization level
        const int optimize = -1;
        
        // create new factory
        m_factory = createDSPFactoryFromString( "chuck", code,
            argc, argv, "", m_errorString, optimize );
        // create DSP instance
        m_dsp = createDSPInstance( m_factory );
        
        // make new UI
        m_ui = new FauckUI();
        // build ui
        m_dsp->buildUserInterface( m_ui );
        
        // allocate
        m_input = new FAUSTFLOAT *[1];
        m_output = new FAUSTFLOAT *[1];
        m_input[0] = new FAUSTFLOAT[1];
        m_output[0] = new FAUSTFLOAT[1];
        
        // init
        m_dsp->init( (int)(m_srate + .5) );
        
        return true;
    }

    // for Chugins extending UGen
    SAMPLE tick( SAMPLE in )
    {
        // sanity check
        if( m_dsp == NULL ) return 0;

        // set input
        m_input[0][0] = in;
        // zero output
        m_output[0][0] = 0;
        // compute samples
        m_dsp->compute( 1, m_input, m_output );
        // return sample
        return m_output[0][0];
    }

    // set parameter example
    t_CKFLOAT setParam( const string & n, t_CKFLOAT p )
    {
        // sanity check
        if( !m_ui ) return 0;

        // set the value
        m_ui->setValue( n, p );
        
        // return
        return p;
    }

    // get parameter example
    t_CKFLOAT getParam()
    { return 0; }
    
private:
    // sample rate
    t_CKFLOAT m_srate;
    // llvm factory
    llvm_dsp_factory * m_factory;
    // faust DSP object
    dsp * m_dsp;
    // faust compiler error string
    string m_errorString;
    
    // faust input buffer
    FAUSTFLOAT ** m_input;
    FAUSTFLOAT ** m_output;
    
    // UI
    FauckUI * m_ui;
};




//-----------------------------------------------------------------------------
// query function: chuck calls this when loading the Chugin
// NOTE: developer will need to modify this function to
// add additional functions to this Chugin
//-----------------------------------------------------------------------------
CK_DLL_QUERY( Faust )
{
    // hmm, don't change this...
    QUERY->setname(QUERY, "Faust");
    
    // begin the class definition
    // can change the second argument to extend a different ChucK class
    QUERY->begin_class(QUERY, "Faust", "UGen");

    // register the constructor (probably no need to change)
    QUERY->add_ctor(QUERY, faust_ctor);
    // register the destructor (probably no need to change)
    QUERY->add_dtor(QUERY, faust_dtor);
    
    // for UGen's only: add tick function
    QUERY->add_ugen_func(QUERY, faust_tick, NULL, 1, 1);
    
    // NOTE: if this is to be a UGen with more than 1 channel, 
    // e.g., a multichannel UGen -- will need to use add_ugen_funcf()
    // and declare a tickf function using CK_DLL_TICKF

    // add .eval()
    QUERY->add_mfun(QUERY, faust_eval, "int", "eval");
    // add argument
    QUERY->add_arg(QUERY, "string", "code");

    // add .test()
    QUERY->add_mfun(QUERY, faust_test, "int", "test");
    // add argument
    QUERY->add_arg(QUERY, "int", "code");

    // add .v()
    QUERY->add_mfun(QUERY, faust_v_set, "float", "v");
    // add arguments
    QUERY->add_arg(QUERY, "string", "key");
    QUERY->add_arg(QUERY, "float", "value");

    // add .v()
    QUERY->add_mfun(QUERY, faust_v_get, "float", "v");
    // add argument
    QUERY->add_arg(QUERY, "string", "key");

    // add .ok()
    QUERY->add_mfun(QUERY, faust_ok, "int", "ok");

    // add .error()
    QUERY->add_mfun(QUERY, faust_error, "string", "error");

    // add .current()
    QUERY->add_mfun(QUERY, faust_code, "string", "code");
    
    // this reserves a variable in the ChucK internal class to store 
    // referene to the c++ class we defined above
    faust_data_offset = QUERY->add_mvar(QUERY, "int", "@f_data", false);

    // end the class definition
    // IMPORTANT: this MUST be called!
    QUERY->end_class(QUERY);

    // wasn't that a breeze?
    return TRUE;
}




// implementation for the constructor
CK_DLL_CTOR(faust_ctor)
{
    // get the offset where we'll store our internal c++ class pointer
    OBJ_MEMBER_INT(SELF, faust_data_offset) = 0;
    
    // instantiate our internal c++ class representation
    Faust * f_obj = new Faust(API->vm->get_srate());
    
    // store the pointer in the ChucK object member
    OBJ_MEMBER_INT(SELF, faust_data_offset) = (t_CKINT) f_obj;
}


// implementation for the destructor
CK_DLL_DTOR(faust_dtor)
{
    // get our c++ class pointer
    Faust * f_obj = (Faust *) OBJ_MEMBER_INT(SELF, faust_data_offset);
    // check it
    if( f_obj )
    {
        // clean up
        delete f_obj;
        OBJ_MEMBER_INT(SELF, faust_data_offset) = 0;
        f_obj = NULL;
    }
}


// implementation for tick function
CK_DLL_TICK(faust_tick)
{
    // get our c++ class pointer
    Faust * f_obj = (Faust *) OBJ_MEMBER_INT(SELF, faust_data_offset);
 
    // invoke our tick function; store in the magical out variable
    if(f_obj) *out = f_obj->tick(in);

    // yes
    return TRUE;
}

CK_DLL_MFUN(faust_eval)
{
    // get our c++ class pointer
    Faust * f = (Faust *) OBJ_MEMBER_INT(SELF, faust_data_offset);
    // get argument
    std::string code = GET_NEXT_STRING(ARGS)->str;
    // eval it
    RETURN->v_int = f->eval( code );
}

CK_DLL_MFUN(faust_test)
{
    // get our c++ class pointer
    Faust * f = (Faust *) OBJ_MEMBER_INT(SELF, faust_data_offset);
    // get argument
    t_CKINT x = GET_NEXT_INT(ARGS);
    // print
    cerr << "TEST: " << x << endl;
    // eval it
    RETURN->v_int = x;
}

CK_DLL_MFUN(faust_compile)
{
}

CK_DLL_MFUN(faust_v_set)
{
    // get our c++ class pointer
    Faust * f = (Faust *)OBJ_MEMBER_INT(SELF, faust_data_offset);
    // get name
    std::string name = GET_NEXT_STRING(ARGS)->str;
    // get value
    t_CKFLOAT v = GET_NEXT_FLOAT(ARGS);
    // call it
    f->setParam( name, v );
    // eval it
    RETURN->v_int = v;
}

CK_DLL_MFUN(faust_v_get)
{
}

CK_DLL_MFUN(faust_ok)
{
}

CK_DLL_MFUN(faust_error)
{
}

CK_DLL_MFUN(faust_code)
{
}
