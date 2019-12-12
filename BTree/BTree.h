#pragma once

#include <vector>
#include <cassert>


template< class Ty >
struct NodeBTree
{
    NodeBTree( size_t size )
    {
        m_M = size;
        m_data.reserve( size );
        m_children.reserve( size );
    }
    ~NodeBTree()
    {
        for( auto& c : m_children )
            delete c;
    }
    bool isLeaf() const
    {
        return m_children.empty();
    }
    size_t countKey() const
    {
        return m_data.size();
    }
    void insert( const Ty& value )
    {
        for( size_t i = 0; i < m_data.size(); ++i )
        {
            if( value < m_data[i] )
            {
                m_data.emplace( m_data.begin() + i, value );
                return;
            }
        }
        m_data.emplace_back( value );
    }
    void removeKey( size_t pos )
    {
        assert( isLeaf() );
        m_data.erase( m_data.begin() + pos );
    }
    void removeKeyAndRightChild( size_t pos )
    {
        m_data.erase( m_data.begin() + pos );
        delete *( m_children.begin() + pos + 1 );
        m_children.erase( m_children.begin() + pos + 1 );
    }
    bool full() const
    {
        return m_data.size() == m_M - 1;
    }
    bool overflowed() const
    {
        return m_data.size() == m_M;
    }
    bool underflowed() const
    {
        return m_data.size() < ( m_M + 1 ) / 2 - 1;
    }
    bool verge_of_underflowed() const
    {
        return m_data.size() == ( m_M + 1 ) / 2 - 1;
    }
    size_t midPos() const
    {
        return (m_M - 1)/2;
    }

    size_t m_M;
    std::vector< Ty > m_data;
    std::vector< NodeBTree< Ty >* > m_children;
    NodeBTree< Ty >* m_parent = nullptr;
};


template< class Ty >
class BTree
{
public:
    BTree( size_t maxSize )
        : m_M( maxSize )
    {
    }
    ~BTree()
    {
        delete m_root;
    }
    bool contains( const Ty& value ) const
    {
        NodeBTree< Ty >* curr = m_root;

        while( true )
        {
            bool check_last = true;
            for( size_t i = 0; i < curr->m_data.size(); ++i )
            {
                auto&& v = curr->m_data[i];
                if( v == value )
                    return true;

                // Текущее значение больше, значит надо смотреть в поддереве
                if( v > value )
                {
                    // Поддерева нету, значит и элемента нету
                    if( curr->m_children.empty() )
                        return false;

                    check_last = false;
                    curr = curr->m_children[i];
                    break;
                }
            }
            if( check_last )
            {
                // Мб есть самое правое поддерево?
                if( curr->m_children.empty() )
                    return false;

                curr = curr->m_children.back();
            }
        }

        return false;
    }
    void insert( const Ty& value )
    {
        auto curr = findNode( value );
        curr->insert( value );
        adjust( curr );
    }
    bool remove( const Ty& value )
    {
        auto[node, pos] = findNodeAndPos( value );
        if( node == nullptr )
            return false;

        if( node->isLeaf() )
        {
            node->removeKey( pos );
            adjust( node );
        }
        else
        {
            NodeBTree< Ty >* n = node->m_children[pos + 1];
            while( !n->isLeaf() )
            {
                n = n->m_children[0];
            }
            node->m_data[pos] = n->m_data[0];
            n->removeKey( 0 );
            adjust( n );
        }

        return true;
    }
private:

    // move value left to right node via parent
    void LR_Redistribute( NodeBTree< Ty >* leftNode )
    {
        //          parent                                          parent
        //       +-----+-+-+-+-----+                             +-----+-+-+-+-----+
        //       | ... |u|x|z| ... |                             | ... |u|w|z| ... |
        //       +-----+-+-+-+-----+                             +-----+-+-+-+-----+
        //              /   \                                           /   \
        //  leftNode   /     \  RightNode      ====>        leftNode   /     \  RightNode
        //   +-----+-+-+     +-+-----+                       +-----+-+        +-+-+-----+
        //   | ... |v|w|     |y| ... |                       | ... |v|        |x|y| ... |
        //   +-----+-+-+     +-+-----+                       +-----+-+        +-+-+-----+
        //         | | |     | |                                   | |        | | |
        //         D E F     G H                                   D E        F G H
        auto posCurr = getPosInParent( leftNode );
        auto parent = leftNode->m_parent;
        auto rigthNode = parent->m_children[posCurr + 1];

        rigthNode->m_data.insert( rigthNode->m_data.begin(), parent->m_data[posCurr] );
        if( !rigthNode->isLeaf() )
        {
            leftNode->m_children.back()->m_parent = rigthNode;
            rigthNode->m_children.insert( rigthNode->m_children.begin(), leftNode->m_children.back() );
        }

        parent->m_data[posCurr] = leftNode->m_data.back();

        leftNode->m_data.pop_back();
        if( !leftNode->isLeaf() )
            leftNode->m_children.pop_back();
    }

    // move value right to left node via parent
    void RL_Redistribute( NodeBTree< Ty >* rightNode )
    {
        //           parent                                    parent
        //        +-----+-+-+-+-----+                       +-----+-+-+-+-----+
        //        | ... |u|w|z| ... |                       | ... |u|x|z| ... |
        //        +-----+-+-+-+-----+                       +-----+-+-+-+-----+
        //               /   \                                     /   \
        //  lleftNode   /     \  RightNode      ====>  leftNode   /     \  RightNode
        //    +-----+-+        +-+-+-----+              +-----+-+-+     +-+-----+
        //    | ... |v|        |x|y| ... |              | ... |v|w|     |y| ... |
        //    +-----+-+        +-+-+-----+              +-----+-+-+     +-+-----+
        //          | |        | | |                          | | |     | |
        //          D E        F G H                          D E F     G H
        auto posCurr = getPosInParent( rightNode );
        auto parent = rightNode->m_parent;
        auto leftNode = parent->m_children[posCurr - 1];

        leftNode->m_data.emplace_back( parent->m_data[posCurr - 1] );
        if( !leftNode->isLeaf() )
        {
            rightNode->m_children.front()->m_parent = leftNode;
            leftNode->m_children.emplace_back( rightNode->m_children.front() );
        }

        parent->m_data[posCurr - 1] = rightNode->m_data.front();

        // Удаляем из донора
        rightNode->m_data.erase( rightNode->m_data.begin() );
        if( !rightNode->isLeaf() )
            rightNode->m_children.erase( rightNode->m_children.begin() );
    }

    void concatenate( NodeBTree< Ty >* leftNode )
    {

        //         parent                                        parent
        //      +-----+-+-+-+-----+                           +-----+-+-+-----+
        //      | ... |u|w|y| ... |                           | ... |u|y| ... |
        //      +-----+-+-+-+-----+                           +-----+-+-+-----+
        //             /   \                                          |
        // leftNode   /     \  RightNode    ====>            leftNode |
        //    +-----+-+     +-+-----+                        +-----+-+-+-+-----+
        //    | ... |v|     |x| ... |                        | ... |v|w|x| ... |
        //    +-----+-+     +-+-----+                        +-----+-+-+-+-----+
        //            |     |                                        | |
        //            E     F                                        E F
        auto posLeft = getPosInParent( leftNode );
        size_t posRight = posLeft + 1;
        auto parent = leftNode->m_parent;
        NodeBTree< Ty >* rightNode = parent->m_children[posRight];

        leftNode->m_data.emplace_back( parent->m_data[posLeft] );
        leftNode->m_data.insert( leftNode->m_data.end(), rightNode->m_data.begin(), rightNode->m_data.end() );
        rightNode->m_data.clear();
        if( !leftNode->isLeaf() )
        {
            leftNode->m_children.insert( leftNode->m_children.end(), rightNode->m_children.begin(), rightNode->m_children.end() );
            for( auto& c : rightNode->m_children )
            {
                c->m_parent = leftNode;
            }
            rightNode->m_children.clear();
        }
        parent->removeKeyAndRightChild( posLeft );
    }

    void split( NodeBTree< Ty >* curr )
    {
        //         parent                                        parent
        //      +-----+-+-+-+-----+                           +-----+-+-+-----+
        //      | ... |u|w|y| ... |                           | ... |u|y| ... |
        //      +-----+-+-+-+-----+                           +-----+-+-+-----+
        //             /   \                                          |
        // curr       /     \  newNode      <====            curr     |
        //    +-----+-+     +-+-----+                        +-----+-+-+-+-----+
        //    | ... |v|     |x| ... |                        | ... |v|w|x| ... |
        //    +-----+-+     +-+-----+                        +-----+-+-+-+-----+
        //            |     |                                        | |
        //            E     F                                        E F

        // Если это рут, то создаем новый рут и делаем старый рут child'ом нового
        if( curr == m_root )
        {
            m_root = new NodeBTree<Ty>( m_M );
            m_root->m_children.emplace_back( curr );
            curr->m_parent = m_root;
        }
        auto parent = curr->m_parent;

        // Делим ровно пополам
        size_t mid = curr->midPos();
        auto value = curr->m_data[mid];

        NodeBTree< Ty >* newNode = new NodeBTree<Ty>( m_M );
        newNode->m_data.assign( curr->m_data.begin() + mid + 1, curr->m_data.end() );
        if( !curr->isLeaf() )
        {
            newNode->m_children.assign( curr->m_children.begin() + mid + 1, curr->m_children.end() );
            for( auto& c : newNode->m_children )
                c->m_parent = newNode;
        }
        newNode->m_parent = curr->m_parent;

        // Удаляем скопированные данные из текущей ноды
        curr->m_data.erase( curr->m_data.begin() + mid, curr->m_data.end() );
        if( !curr->isLeaf() )
            curr->m_children.erase( curr->m_children.begin() + mid + 1, curr->m_children.end() );

        auto pos = getPosInParent( curr );
        // вставляем в родителя новую ноду
        parent->m_data.insert( parent->m_data.begin() + pos, value );
        parent->m_children.insert( parent->m_children.begin() + pos + 1, newNode );
    }

    void adjust( NodeBTree< Ty >* node )
    {
        // Переполенно
        if( node->overflowed() )
        {
            // Пытаемся переместить вправо
            if( auto rightSibling = getRightSibling( node ); rightSibling != nullptr && !rightSibling->full() )
            {
                LR_Redistribute(node);
            }// или влево
            else if( auto leftSibling = getLeftSibling( node ); leftSibling != nullptr && !leftSibling->full() )
            {
                RL_Redistribute( node );
            }
            else
            {
                // ну или делим
                split( node );
                adjust( node->m_parent );
            }
        }
        // Нехватает
        else if( node->underflowed() )
        {
            // Может быть можно стащить у левого
            if( auto leftSibling = getLeftSibling( node ); leftSibling != nullptr && !leftSibling->verge_of_underflowed() )
            {
                LR_Redistribute( leftSibling );
            }
            // или у правого
            else if( auto rightSibling = getRightSibling( node ); rightSibling != nullptr && !rightSibling->verge_of_underflowed() )
            {
                RL_Redistribute( rightSibling );
            }
            // а может быть это рут, тогда законно
            else if( m_root == node )
            {
                if( node->m_children.size() == 1 )
                {
                    m_root = node->m_children[0];
                    m_root->m_parent = nullptr;
                    node->m_children.clear();
                    delete node;
                }
            }
            else
            {
                // ничего ни у кого лишнего нет, значит будем объединять
                if( auto rightSibling = getRightSibling( node ); rightSibling != nullptr )
                {
                    concatenate( node );
                    adjust( node->m_parent );
                }
                else
                {
                    auto leftSibling = getLeftSibling( node );
                    concatenate( leftSibling );
                    adjust( leftSibling->m_parent );
                }
            }
        }
        else
        {
            // its ok
        }
    }

    NodeBTree< Ty >* findNode( const Ty& value )
    {
        NodeBTree< Ty >* curr = m_root;
        // Ищем самый нижний уровень
        while( !curr->m_children.empty() )
        {
            size_t i;
            bool founded = false;
            for( i = 0; i < curr->m_data.size(); ++i )
            {
                auto&& v = curr->m_data[i];
                if( value < v )
                {
                    curr = curr->m_children[i];
                    founded = true;
                    break;
                }
            }
            if( !founded )
                curr = curr->m_children.back();
        }

        return curr;
    }
    // Находим ноду и позицию в родителе
    std::pair< NodeBTree< Ty >*, size_t > findNodeAndPos( const Ty& value )
    {
        NodeBTree< Ty >* curr = m_root;
        while( curr )
        {
            size_t i;
            bool founded = false;
            for( i = 0; i < curr->m_data.size(); ++i )
            {
                auto&& v = curr->m_data[i];
                if( value == v )
                {
                    return std::make_pair( curr, i );
                }
                if( value < v )
                {
                    founded = true;
                    if( curr->isLeaf() )
                        curr = nullptr;
                    else
                        curr = curr->m_children[i];
                    break;
                }
            }
            if( !founded )
                curr = curr->isLeaf() ? nullptr : curr->m_children.back();
        }

        return std::make_pair( nullptr, 0 );
    }

    // Получить позицу в родителе
    size_t getPosInParent( NodeBTree< Ty >* curr )
    {
        NodeBTree< Ty >* parent = curr->m_parent;
        auto iterCurr = std::find( parent->m_children.begin(), parent->m_children.end(), curr );

        return std::distance( parent->m_children.begin(), iterCurr );
    }

    NodeBTree< Ty >* getLeftSibling( NodeBTree<Ty>* curr, size_t pos )
    {
        if( pos == 0 )
            return nullptr;

        return curr->m_parent->m_children[pos - 1];
    }

    NodeBTree< Ty >* getLeftSibling( NodeBTree<Ty>* curr )
    {
        if( curr->m_parent == nullptr )
            return nullptr;

        size_t pos = getPosInParent( curr );
        return getLeftSibling( curr, pos );
    }

    NodeBTree< Ty >* getRightSibling( NodeBTree<Ty>* curr, size_t pos )
    {
        if( pos + 1 == curr->m_parent->m_children.size() )
            return nullptr;

        return curr->m_parent->m_children[pos + 1];
    }

    NodeBTree< Ty >* getRightSibling( NodeBTree<Ty>* curr )
    {
        if( curr->m_parent == nullptr )
            return nullptr;

        size_t pos = getPosInParent( curr );
        return getRightSibling( curr, pos );
    }

    size_t m_M = 0;
    NodeBTree< Ty >* m_root = new NodeBTree< Ty >( m_M );
};
