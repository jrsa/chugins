#include "chuck_dl.h"
#include "chuck_def.h"

#include <stdio.h>
#include <limits.h>
#include <string.h>

CK_DLL_CTOR(ballpass69_ctor);
CK_DLL_DTOR(ballpass69_dtor);

CK_DLL_MFUN(ballpass69_setOrder);
CK_DLL_MFUN(ballpass69_getOrder);
CK_DLL_MFUN(ballpass69_setG);
CK_DLL_MFUN(ballpass69_getG);

CK_DLL_TICK(ballpass69_tick);

t_CKINT ballpass69_data_offset = 0;

class BallPass69 {
public:
  BallPass69(t_CKFLOAT fs)
      : m_ff(NULL), m_fb(NULL), m_g(0.7), m_order(200), ff_read(0),
        ff_write(0) {

    ff_write = m_order;
  }

  ~BallPass69() {
    if (m_ff)
      free(m_ff);
    if (m_fb)
      free(m_fb);
  }

  SAMPLE tick(SAMPLE in) {
    // only doing feedforward rn
    m_ff[ff_write++] = in;

    if (ff_read > m_order) {
      ff_read = 0;
    }
    SAMPLE ff_xn = m_ff[ff_read];

    if (ff_write > m_order) {
      ff_write = 0;
    }

    // difference equation for feedforward comb section
    return (m_g * ff_xn) + in;
  }

  int setOrder(t_CKINT _order) {
    m_order = _order;

    printf("setting order to %u\n", m_order);

    // re alloc buffers
    m_ff = (SAMPLE *)malloc(sizeof(SAMPLE) * m_order);
    m_fb = (SAMPLE *)malloc(sizeof(SAMPLE) * m_order);

    memset(m_ff, 0, m_order);
    memset(m_fb, 0, m_order);

    return m_order;
  }

  int getOrder() { return m_order; }

  float setG(float g) {
    m_g = g;
    return m_g;
  }

private:
  int m_order;
  float m_g;
  SAMPLE *m_ff, *m_fb;

  int ff_read, fb_read;
  int ff_write, fb_write;
};

CK_DLL_QUERY(BallPass69) {
  QUERY->setname(QUERY, "BallPass69");
  QUERY->begin_class(QUERY, "BallPass69", "UGen");

  QUERY->add_ctor(QUERY, ballpass69_ctor);
  QUERY->add_dtor(QUERY, ballpass69_dtor);

  QUERY->add_ugen_func(QUERY, ballpass69_tick, NULL, 1, 1);

  QUERY->add_mfun(QUERY, ballpass69_setG, "float", "g");
  QUERY->add_arg(QUERY, "float", "g");

  QUERY->add_mfun(QUERY, ballpass69_setOrder, "float", "order");
  QUERY->add_arg(QUERY, "float", "order");

  QUERY->add_mfun(QUERY, ballpass69_getOrder, "float", "order");

  ballpass69_data_offset = QUERY->add_mvar(QUERY, "int", "@bp69_data", false);

  QUERY->end_class(QUERY);

  return TRUE;
}

CK_DLL_CTOR(ballpass69_ctor) {
  OBJ_MEMBER_INT(SELF, ballpass69_data_offset) = 0;
  BallPass69 *bp69_obj = new BallPass69(API->vm->get_srate());
  OBJ_MEMBER_INT(SELF, ballpass69_data_offset) = (t_CKINT)bp69_obj;
}

CK_DLL_DTOR(ballpass69_dtor) {
  BallPass69 *bp69_obj =
      (BallPass69 *)OBJ_MEMBER_INT(SELF, ballpass69_data_offset);

  if (bp69_obj) {
    delete bp69_obj;
    OBJ_MEMBER_INT(SELF, ballpass69_data_offset) = 0;
    bp69_obj = NULL;
  }
}

CK_DLL_TICK(ballpass69_tick) {
  BallPass69 *bp69_obj =
      (BallPass69 *)OBJ_MEMBER_INT(SELF, ballpass69_data_offset);

  if (bp69_obj)
    *out = bp69_obj->tick(in);

  return TRUE;
}

CK_DLL_MFUN(ballpass69_setOrder) {
  BallPass69 *bp69_obj =
      (BallPass69 *)OBJ_MEMBER_INT(SELF, ballpass69_data_offset);
  RETURN->v_int = bp69_obj->setOrder(GET_NEXT_INT(ARGS));
}

CK_DLL_MFUN(ballpass69_getOrder) {
  BallPass69 *bp69_obj =
      (BallPass69 *)OBJ_MEMBER_INT(SELF, ballpass69_data_offset);
  RETURN->v_int = bp69_obj->getOrder();
}

CK_DLL_MFUN(ballpass69_setG) {
  BallPass69 *bp69_obj =
      (BallPass69 *)OBJ_MEMBER_INT(SELF, ballpass69_data_offset);
  RETURN->v_int = bp69_obj->setG(GET_NEXT_INT(ARGS));
}
