#include <vector>
#include <numeric>
#include <random>
#include <algorithm>
#include <iostream>
#include <fstream>

#include "BTree.h"

int main()
{
    try
    {
        size_t maxCountTestData = 1000;
        size_t maxIteration =10;
        for( size_t sizeNode = 3; sizeNode <= 10; ++sizeNode )
        {
            std::cout << "Testing SizeNode = " << sizeNode << "...";
            for( size_t countData = 10; countData < maxCountTestData; ++countData )
            {
                for( size_t j = 0; j < maxIteration; ++j )
                {
                    size_t size = countData;
                    // ���������� �������� ������
                    std::vector< int > testData( size );
                    std::iota( testData.begin(), testData.end(), 1 );
                    std::mt19937 mt( j + countData * maxIteration );
                    std::shuffle( testData.begin(), testData.end(), mt );

                    BTree<int> tree( sizeNode );
                    // ��������� ��� �������� ������
                    for( size_t i = 0; i < testData.size(); ++i )
                    {
                        tree.insert( testData[i] );
                    }

                    // ���������, ��� ��� ��������
                    for( size_t i = 0; i < testData.size(); ++i )
                    {
                        if( !tree.contains( testData[i] ) )
                            throw std::runtime_error( "fail insert" );
                    }

                    // ������� � ����� ��������, ��� �������
                    for( size_t i = 0; i < testData.size(); ++i )
                    {
                        if( !tree.remove( testData[i] ) )
                            throw std::runtime_error( "fail deleting" );
                        if( tree.contains( testData[i] ) )
                            throw std::runtime_error( "fail deleting" );
                    }
                    // �������� ������� ��������, ������ ������� false
                    for( size_t i = 0; i < testData.size(); ++i )
                    {
                        if( tree.remove( testData[i] ) )
                            throw std::runtime_error( "fail work deleting" );
                        if( tree.contains( testData[i] ) )
                            throw std::runtime_error( "fail" );
                    }
                }
            }
            std::cout << "OK" << std::endl;
        }
    }
    catch( const std::exception& e )
    {
        std::cerr << "Fail testing: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}