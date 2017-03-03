/*
 oaa.h
 Andrew J Wood
 COP 4530
 March 1, 2017
 
 This header file defines the Ordered Associative Array API and then implements it using a RBLLT
 (Red Black Left Leaning Tree). The purpose of the OAA is to function as a Table with the addition
 of a [] operator for retrievals.  In addition, the runtimes of all operations will be Theta(log n)
 with the exception of the Rehash() function, which will be Thetat(n log n).  The RBLLT structure is
 what ensures the log n runtimes; the rehash fucntion is n log n because it requires a full tree
 traversal and new tree creation.
 */

#ifndef _OAA_H
#define _OAA_H

#include <cstddef>    // size_t
#include <iostream>
#include <iomanip>
#include <compare.h>  // LessThan
#include <queue.h>    // used in Dump()
#include <ansicodes.h>

namespace fsu
{
    template < typename K , typename D , class P >
    class OAA;
    
    template < typename K , typename D , class P = LessThan<K> >
    class OAA
    {
    public:
        
        typedef K    KeyType;
        typedef D    DataType;
        typedef P    PredicateType;
        
        OAA  ();
        explicit OAA  (P p);
        OAA  (const OAA& a);
        ~OAA ();
        OAA& operator=(const OAA& a);
        
        DataType& operator [] (const KeyType& k)        { return Get(k); }
        
        void Put (const KeyType& k , const DataType& d) { Get(k) = d; }
        D&   Get (const KeyType& k);
        
        void Erase(const KeyType& k);
        void Clear();
        void Rehash();
        
        bool   Empty    () const { return root_ == nullptr; }
        size_t Size     () const { return RSize(root_); }     // counts alive nodes
        size_t NumNodes () const { return RNumNodes(root_); } // counts nodes
        int    Height   () const { return RHeight(root_); }
        
        template <class F>
        void   Traverse(F f) const { RTraverse(root_,f); }
        
        void   Display (std::ostream& os, int kw, int dw,     // key, data widths
                        std::ios_base::fmtflags kf = std::ios_base::right, // key flag
                        std::ios_base::fmtflags df = std::ios_base::right // data flag
        ) const;
        
        void   DumpBW (std::ostream& os) const;
        void   Dump (std::ostream& os) const;
        void   Dump (std::ostream& os, int kw) const;
        void   Dump (std::ostream& os, int kw, char fill) const;
        
    private: // definitions and relationships
        
        enum Flags { ZERO = 0x00 , DEAD = 0x01, RED = 0x02 , DEFAULT = RED }; // DEFAULT = alive,red
        static const char* ColorMap (unsigned char flags)
        {
            switch(flags)
            {
                case 0x00: return ANSI_BOLD_BLUE;        // bits 00
                case 0x01: return ANSI_BOLD_BLUE_SHADED; // bits 01
                case 0x02: return ANSI_BOLD_RED;         // bits 10
                case 0x03: return ANSI_BOLD_RED_SHADED;  // bits 11
                default: return "unknown color";   // unknown flags
            }
        }
        
        static char BWMap (uint8_t flags)
        {
            switch(flags)
            {
                case 0x00: return 'B'; // bits 00 black alive
                case 0x01: return 'b'; // bits 01 black dead
                case 0x02: return 'R'; // bits 10 red alive
                case 0x03: return 'r'; // bits 11 red dead
                default:   return 'U'; // unknown flags
            }
        }
        
        class Node
        {
            const KeyType   key_;
            DataType  data_;
            Node * lchild_, * rchild_;
            uint8_t flags_; //8 bit value
            Node (const KeyType& k, const DataType& d, Flags flags = DEFAULT) // Flags = RED, Alive
            : key_(k), data_(d), lchild_(nullptr), rchild_(nullptr), flags_(flags)
            {}
            friend class OAA<K,D,P>;
            bool IsRed    () const { return 0 != (RED & flags_); }
            bool IsBlack  () const { return !IsRed(); }
            bool IsDead   () const { return 0 != (DEAD & flags_); }
            bool IsAlive  () const { return !IsDead(); }
            void SetRed   ()       { flags_ |= RED; }
            void SetBlack ()       { flags_ &= ~RED; }
            void SetDead  ()       { flags_ |= DEAD; }
            void SetAlive ()       { flags_ &= ~DEAD; }
            
            //additional helper methods for clarity
            bool RightChildIsRed() const
            {
                if (rchild_)
                    return rchild_->IsRed();
                else //if there is no right child
                    return 0;
            }
            
            bool LeftChildIsRed() const
            {
                if (lchild_)
                    return lchild_->IsRed();
                else
                    return 0;
            }
            
        };
        
        class PrintNode
        {
        public:
            PrintNode (std::ostream& os, int kw, int dw,
                       std::ios_base::fmtflags kf, std::ios_base::fmtflags df )
            : os_(os), kw_(kw), dw_(dw), kf_(kf), df_(df) {}
            void operator() (const Node * n) const
            {
                if (n->IsAlive())
                {
                    os_.setf(kf_,std::ios_base::adjustfield);
                    os_ << std::setw(kw_) << n->key_;
                    os_.setf(df_,std::ios_base::adjustfield);
                    os_ << std::setw(dw_) << n->data_;
                    os_ << '\n';
                }
            }
        private:
            std::ostream& os_;
            int kw_, dw_;      // key and data column widths
            std::ios_base::fmtflags kf_, df_; // column adjustment flags for output
            //stream
        };
        
        class CopyNode
        {
        public:
            CopyNode (Node*& newroot, OAA<K,D>* oaa) : newroot_(newroot), oldtree_(oaa) {}
            void operator() (const Node * n) const
            {
                if (n->IsAlive())
                {
                    newroot_ = oldtree_->RInsert(newroot_,n->key_, n->data_);
                    newroot_->SetBlack();
                }
            }
        private:
            Node *&    newroot_;
            OAA<K,D> * oldtree_;
        };
        
    private: // data
        Node *         root_;
        PredicateType  pred_;
        
    private: // methods
        static Node * NewNode     (const K& k, const D& d, Flags flags = DEFAULT);
        static void   RRelease    (Node* n); // deletes all descendants of n
        static Node * RClone      (const Node* n); // returns deep copy of n
        static size_t RSize       (Node * n);
        static size_t RNumNodes   (Node * n);
        static int    RHeight     (Node * n);
        
        // rotations
        static Node * RotateLeft  (Node * n);
        static Node * RotateRight (Node * n);
        
        template < class F >
        static void   RTraverse (Node * n, F f);
        
        // recursive left-leaning get
        Node * RGet(Node* nptr, const K& kval, Node*& location);
        
        // recursive left-leaning insert
        Node * RInsert(Node* nptr, const K& key, const D& data);
        
    }; // class OAA<>
    
    
    // API
    
    //1//
    template < typename K , typename D , class P >
    D& OAA<K,D,P>::Get (const KeyType& k)
    {
        //returns reference to data value assoated with k; inserts if necessary
        Node * location;
        root_ = RGet(root_,k,location); //use recursive get to find location of key
        root_ -> SetBlack(); //root is always black
        return location->data_; //returns node's data as a reference
    }
    
    //EC//
    template < typename K , typename D , class P >
    void OAA<K,D,P>::Erase(const KeyType& k)
    {
        Node * n = root_; // start at root of tree
        while(n) //while on a valid node
        {
            if (pred_(k, n->key_)) //if k is less than current key
            {
                n = n->lchild_; //go left
            }
            else if (pred_(n->key_, k)) //if k is greater than current key
            {
                n = n->rchild_; //go right
            }
            else //key found
            {
                n->SetDead();
                return;
            }
        }
    }
    
    //2//
    template < typename K , typename D , class P >
    void OAA<K,D,P>::Clear()
    {
        RRelease(root_); //delete all descendents of root
        delete root_; //delete the root itself
        root_ = 0; //set root to 0 (empty tree)
    }
    
    template < typename K , typename D , class P >
    void OAA<K,D,P>::Rehash()
    { // this is complete!
        Node* newRoot = nullptr;
        CopyNode cn(newRoot,this);
        Traverse(cn);
        Clear();
        root_ = newRoot;
    }
    
    //3//
    template < typename K , typename D , class P >
    void  OAA<K,D,P>::Display (std::ostream& os, int kw, int dw, std::ios_base::fmtflags kf, std::ios_base::fmtflags df) const
    {
        PrintNode pn(os, kw, dw, kf, df);  //create print node object
        Traverse(pn); //traverse using PrintNode function object
    }
    
    //4//
    template < typename K , typename D , class P >
    typename OAA<K,D,P>::Node * OAA<K,D,P>::RGet(Node* nptr, const K& kval, Node*& location)
    // recursive left-leaning get; returns node location of found value
    {
        if (nptr == 0) //add new node at "bottom" of tree
        {
            location = NewNode(kval, D()); //note, will use DEFAULT as flags argument (RED and ALIVE)
            return location;
        }
        if (pred_(kval,nptr->key_)) //if kval < key_ in current node, go to left subtree
        {
            nptr->lchild_ = RGet(nptr->lchild_,kval,location);
        }
        else if (pred_(nptr->key_,kval)) // if kval > key_ in current node, go to right subtree
        {
            nptr->rchild_ = RGet(nptr->rchild_,kval,location);
        }
        else // the node exists and was found; set location only, don't update value
        {
            location = nptr;
            nptr -> SetAlive(); //set alive; Get will insert if data is not found, hence if any node
                                //is found containing the data it should be set to alive.
        }
        
        
        //repair the RBLL properties on the way up
        if (nptr->RightChildIsRed() && !nptr->LeftChildIsRed()) //if the right child is red but left is not
            nptr = RotateLeft(nptr); //rotate the tree left around nptr; nptr has replacement
        if (nptr->LeftChildIsRed() && nptr->lchild_->LeftChildIsRed()) //if there are two consecutive red left ndoes
            nptr = RotateRight(nptr); //rotate right
        if (nptr->LeftChildIsRed() && nptr->RightChildIsRed()) //red node has to have only black children
        {   //swap parent/child colors
            nptr->lchild_->SetBlack();
            nptr->rchild_->SetBlack();
            nptr->SetRed();
        }
        
        return nptr; //returns root location;
    }
    
    
    //5//
    template < typename K , typename D , class P >
    typename OAA<K,D,P>::Node * OAA<K,D,P>::RInsert(Node* nptr, const K& key, const D& data)
    // recursive left-leaning insert; very similar to RGet
    {
        if (nptr == 0) //add new node at "bottom" of tree
        {
            return NewNode(key, data); //note, will use DEFAULT as flags argument (RED)
        }
        if (pred_(key,nptr->key_)) //if kval < key_ in current node, go to left subtree
        {
            nptr->lchild_ = RInsert(nptr->lchild_,key,data);
        }
        else if (pred_(nptr->key_,key)) // if kval > key_ in current node, go to right subtree
        {
            nptr->rchild_ = RInsert(nptr->rchild_,key,data);
        }
        else // the node exists and was found; overwrite data
        {
            nptr->data_ = data; //overright data at corresponding key; note key is constant and cannot be overwritten
            nptr->SetAlive(); //set Alive in case it is not already alive
        }
        
        //repair the RBLL properties on the way up
        if (nptr->RightChildIsRed() && !nptr->LeftChildIsRed()) //if the right child is red but left is not
            nptr = RotateLeft(nptr); //rotate the tree left around nptr; nptr has replacement
        if (nptr->LeftChildIsRed() && nptr->lchild_->LeftChildIsRed()) //if there are two consecutive red left ndoes
            nptr = RotateRight(nptr); //rotate right
        if (nptr->LeftChildIsRed() && nptr->RightChildIsRed()) //red node has to have only black children
        {   //swap parent/child colors
            nptr->lchild_->SetBlack();
            nptr->rchild_->SetBlack();
            nptr->SetRed();
        }
        
        return nptr; //returns root location;
    }
    
    
    /************************************/
    /* everyting below here is complete */
    
    // proper type
    
    template < typename K , typename D , class P >
    OAA<K,D,P>::OAA  () : root_(nullptr), pred_()
    {}
    
    template < typename K , typename D , class P >
    OAA<K,D,P>::OAA  (P p) : root_(nullptr), pred_(p)
    {}
    
    template < typename K , typename D , class P >
    OAA<K,D,P>::~OAA ()
    {
        Clear();
    }
    
    template < typename K , typename D , class P >
    OAA<K,D,P>::OAA( const OAA& tree ) : root_(nullptr), pred_(tree.pred_)
    {
        root_ = RClone(tree.root_);
    }
    
    template < typename K , typename D , class P >
    OAA<K,D,P>& OAA<K,D,P>::operator=( const OAA& that )
    {
        if (this != &that)
        {
            Clear();
            this->root_ = RClone(that.root_);
        }
        return *this;
    }
    
    // rotations
    template < typename K , typename D , class P >
    typename OAA<K,D,P>::Node * OAA<K,D,P>::RotateLeft(Node * n)
    {
        if (nullptr == n || n->rchild_ == nullptr) return n;
        if (!n->rchild_->IsRed())
        {
            std::cerr << " ** RotateLeft called with black right child\n";
            return n;
        }
        Node * p = n->rchild_;
        n->rchild_ = p->lchild_;
        p->lchild_ = n;
        
        n->IsRed()? p->SetRed() : p->SetBlack();
        n->SetRed();
        return p;
    }
    
    template < typename K , typename D , class P >
    typename OAA<K,D,P>::Node * OAA<K,D,P>::RotateRight(Node * n)
    {
        if (n == nullptr || n->lchild_ == nullptr) return n;
        if (!n->lchild_->IsRed())
        {
            std::cerr << " ** RotateRight called with black left child\n";
            return n;
        }
        
        Node * p = n->lchild_;
        n->lchild_ = p->rchild_;
        p->rchild_ = n;
        
        n->IsRed()? p->SetRed() : p->SetBlack();
        n->SetRed();
        return p;
    }
    
    // private static recursive methods
    
    template < typename K , typename D , class P >
    size_t OAA<K,D,P>::RSize(Node * n)
    {
        if (n == nullptr) return 0;
        return (size_t)(n->IsAlive()) + RSize(n->lchild_) + RSize(n->rchild_);
    }
    
    template < typename K , typename D , class P >
    size_t OAA<K,D,P>::RNumNodes(Node * n)
    {
        if (n == nullptr) return 0;
        return 1 + RNumNodes(n->lchild_) + RNumNodes(n->rchild_);
    }
    
    template < typename K , typename D , class P >
    int OAA<K,D,P>::RHeight(Node * n)
    {
        if (n == nullptr) return -1;
        int lh = RHeight(n->lchild_);
        int rh = RHeight(n->rchild_);
        if (lh < rh) return 1 + rh;
        return 1 + lh;
    }
    
    template < typename K , typename D , class P >
    template < class F >
    void OAA<K,D,P>::RTraverse (Node * n, F f)
    {
        if (n == nullptr) return;
        RTraverse(n->lchild_,f);
        f(n);
        RTraverse(n->rchild_,f);
    }
    
    template < typename K , typename D , class P >
    void OAA<K,D,P>::RRelease(Node* n)
    // post:  all descendants of n have been deleted
    {
        if (n != nullptr)
        {
            if (n->lchild_ != nullptr)
            {
                OAA<K,D,P>::RRelease(n->lchild_);
                delete n->lchild_;
                n->lchild_ = nullptr;
            }
            if (n->rchild_ != nullptr)
            {
                OAA<K,D,P>::RRelease(n->rchild_);
                delete n->rchild_;
                n->rchild_ = nullptr;
            }
        }
    } // OAA<K,D,P>::RRelease()
    
    template < typename K , typename D , class P >
    typename OAA<K,D,P>::Node* OAA<K,D,P>::RClone(const OAA<K,D,P>::Node* n)
    // returns a pointer to a deep copy of n
    {
        if (n == nullptr)
            return 0;
        typename OAA<K,D,P>::Node* newN = NewNode (n->key_,n->data_);
        newN->flags_ = n->flags_;
        newN->lchild_ = OAA<K,D,P>::RClone(n->lchild_);
        newN->rchild_ = OAA<K,D,P>::RClone(n->rchild_);
        return newN;
    } // end OAA<K,D,P>::RClone() */
    
    
    // private node allocator
    template < typename K , typename D , class P >
    typename OAA<K,D,P>::Node * OAA<K,D,P>::NewNode(const K& k, const D& d, Flags flags)
    {
        Node * nPtr = new(std::nothrow) Node(k,d,flags);
        if (nPtr == nullptr)
        {
            std::cerr << "** OAA memory allocation failure\n";
            // handle exception in-class here
        }
        return nPtr;
    }
    
    // development assistants
    
    template < typename K , typename D , class P >
    void OAA<K,D,P>::DumpBW (std::ostream& os) const
    {
        // fsu::debug ("DumpBW(1)");
        // This is the same as "Dump(1)" except it uses a character map instead of a
        // color map for the display: B/b = black_alive/black_dead, R/r = red_alive/red_dead
        
        if (root_ == nullptr)
            return;
        
        // we will use "root" as the placeholder node; test for dummy by address
        Queue < Node * , Deque < Node * > > Que;
        char nullFill = '-';
        Node * fillNode = root_;
        Node * current;
        
        size_t currLayerSize, nextLayerSize, j, k;
        currLayerSize = 1;
        k = 1;  // 2^LayerNumber
        Que.Push(root_);
        
        // execute body once for "real" root:
        os << ' '; // 1-space left margin for graphic
        nextLayerSize = 0;
        current = Que.Front();
        Que.Pop();
        os << BWMap(current->flags_);
        if (current->lchild_ != nullptr)
        {
            Que.Push(current->lchild_);
            ++nextLayerSize;
        }
        else
        {
            Que.Push(fillNode);
        }
        if (current->rchild_ != nullptr)
        {
            Que.Push(current->rchild_);
            ++nextLayerSize;
        }
        else
        {
            Que.Push(fillNode);
        }
        os << '\n';
        currLayerSize = nextLayerSize;
        k *= 2;
        
        // now finish off, using address to detect fillNode.
        while (currLayerSize > 0)
        {
            nextLayerSize = 0;
            os << ' '; // 1-space left margin for graphic
            for (j = 0; j < k; ++j)
            {
                current = Que.Front();
                Que.Pop();
                if (current == fillNode) // an empty position in the tree
                    os << nullFill;
                else
                    os << BWMap(current->flags_);
                if (current != fillNode && current->lchild_ != nullptr)
                {
                    Que.Push(current->lchild_);
                    ++nextLayerSize;
                }
                else
                {
                    Que.Push(fillNode);
                }
                if (current != fillNode && current->rchild_ != nullptr)
                {
                    Que.Push(current->rchild_);
                    ++nextLayerSize;
                }
                else
                {
                    Que.Push(fillNode);
                }
            } // end for
            os << '\n';
            currLayerSize = nextLayerSize;
            k *= 2;
        } // end while
        Que.Clear();
    } // DumpBW(os)
    
    template < typename K , typename D , class P >
    void OAA<K,D,P>::Dump (std::ostream& os) const
    {
        // fsu::debug ("Dump(1)");
        
        // This implementation is very like that for Dump(os, kw, fill), except that
        // there is no supplied fill object or output format character. The trick is
        // to use the root of a non-empty tree as the fill. To make that work, we have
        // to take care of the real root case before entering the main loop.
        
        if (root_ == nullptr)
            return;
        
        // we will use "root" as the placeholder node; test for dummy by address
        Queue < Node * , Deque < Node * > > Que;
        char nullFill = '-';
        char nodeFill = '*';
        Node * fillNode = root_;
        Node * current;
        
        size_t currLayerSize, nextLayerSize, j, k;
        currLayerSize = 1;
        k = 1;  // 2^LayerNumber
        Que.Push(root_);
        
        // execute body once for "real" root:
        os << ' '; // 1-space left margin for graphic
        nextLayerSize = 0;
        current = Que.Front();
        Que.Pop();
        os << ColorMap(current->flags_) << nodeFill << ANSI_RESET_ALL;
        if (current->lchild_ != nullptr)
        {
            Que.Push(current->lchild_);
            ++nextLayerSize;
        }
        else
        {
            Que.Push(fillNode);
        }
        if (current->rchild_ != nullptr)
        {
            Que.Push(current->rchild_);
            ++nextLayerSize;
        }
        else
        {
            Que.Push(fillNode);
        }
        os << '\n';
        currLayerSize = nextLayerSize;
        k *= 2;
        
        // now finish off, using address to detect fillNode.
        while (currLayerSize > 0)
        {
            nextLayerSize = 0;
            os << ' '; // 1-space left margin for graphic
            for (j = 0; j < k; ++j)
            {
                current = Que.Front();
                Que.Pop();
                if (current == fillNode) // an empty position in the tree
                    os << nullFill;
                else
                    os << ColorMap(current->flags_) << nodeFill << ANSI_RESET_ALL;
                if (current != fillNode && current->lchild_ != nullptr)
                {
                    Que.Push(current->lchild_);
                    ++nextLayerSize;
                }
                else
                {
                    Que.Push(fillNode);
                }
                if (current != fillNode && current->rchild_ != nullptr)
                {
                    Que.Push(current->rchild_);
                    ++nextLayerSize;
                }
                else
                {
                    Que.Push(fillNode);
                }
            } // end for
            os << '\n';
            currLayerSize = nextLayerSize;
            k *= 2;
        } // end while
        Que.Clear();
    } // Dump(os)
    
    template < typename K , typename D , class P >
    void OAA<K,D,P>::Dump (std::ostream& os, int kw) const
    {
        // fsu::debug ("Dump(2)");
        if (root_ == nullptr)
            return;
        Queue < Node * , Deque < Node * > > Que;
        Node * current;
        size_t currLayerSize, nextLayerSize, j;
        Que.Push(root_);
        currLayerSize = 1;
        while (!Que.Empty())
        {
            nextLayerSize = 0;
            if (kw == 1) os << ' '; // indent picture 1 space
            for (j = 0; j < currLayerSize; ++j)
            {
                current = Que.Front();
                Que.Pop();
                if (kw > 1) os << ' '; // indent each column 1 space
                os << ColorMap(current->flags_) << std::setw(kw) << current->key_<< ANSI_RESET_ALL;
                if (current->lchild_ != nullptr)
                {
                    Que.Push(current->lchild_);
                    ++nextLayerSize;
                }
                if (current->rchild_ != nullptr)
                {
                    Que.Push(current->rchild_);
                    ++nextLayerSize;
                }
            }
            os << '\n';
            currLayerSize = nextLayerSize;
        } // end while
        if (currLayerSize > 0)
            std::cerr << "** OAA<K,D,P>::Dump() inconsistency\n";
    } // Dump(os, kw)
    
    template < typename K , typename D , class P >
    void OAA<K,D,P>::Dump (std::ostream& os, int kw, char fill) const
    {
        // fsu::debug ("Dump(3)");
        if (root_ == nullptr)
            return;
        
        Node* fillNode = NewNode(K(),D());
        Queue < Node * , Deque < Node * > > Que;
        Node * current;
        size_t currLayerSize, nextLayerSize, j, k;
        Que.Push(root_);
        currLayerSize = 1;
        k = 1;  // 2^LayerNumber
        while (currLayerSize > 0)
        {
            nextLayerSize = 0;
            if (kw == 1) os << ' '; // indent picture 1 space
            for (j = 0; j < k; ++j)
            {
                current = Que.Front();
                Que.Pop();
                if (kw > 1) os << ' '; // indent each column 1 space
                if (current == fillNode)
                {
                    os <<std::setw(kw) << fill;
                }
                else
                {
                    os << ColorMap(current->flags_) << std::setw(kw) << current->key_<< ANSI_RESET_ALL;
                }
                
                if (current->lchild_ != nullptr)
                {
                    Que.Push(current->lchild_);
                    ++nextLayerSize;
                }
                else
                {
                    Que.Push(fillNode);
                }
                if (current->rchild_ != nullptr)
                {
                    Que.Push(current->rchild_);
                    ++nextLayerSize;
                }
                else
                {
                    Que.Push(fillNode);
                }
            }
            os << '\n';
            currLayerSize = nextLayerSize;
            k *= 2;
        } // end while
        Que.Clear();
        delete fillNode;
    } // Dump(os, kw, fill) */
    
} // namespace fsu 

#endif
