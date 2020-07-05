#ifndef HRESULTERRORS_H
#define HRESULTERRORS_H

#define RET_FAILED(x) do {const auto r = (x); if(FAILED(r)) return r; } while(0);
#define RET_EMPTY(x) do {const auto r = (x); if(FAILED(r)) return {}; } while(0);

#endif // HRESULTERRORS_H
