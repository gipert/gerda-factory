/* GerdaFactory.h
 *
 * Author: Luigi Pertoldi
 * Created: Tue 18 Jun 2019
 *
 */
#ifndef _GERDA_FACTORY_H
#define _GERDA_FACTORY_H

class GerdaFactory {

    public:

    // delete dangerous constructors
    GerdaFactory           (GerdaFactory const&) = delete;
    GerdaFactory& operator=(GerdaFactory const&) = delete;

    // custom constructor
    GerdaFactory();
    ~GerdaFactory();

};

#endif
