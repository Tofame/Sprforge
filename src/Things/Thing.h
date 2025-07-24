#pragma once

#ifndef THING_H
#define THING_H

class Item;

class Thing {
protected:
    Thing() = default;

public:
    virtual Item* getItem() {return nullptr;}
};



#endif //THING_H
