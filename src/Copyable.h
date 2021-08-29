/**
 * 
 */
#pragma once
namespace Hohnor
{
/// A tag class emphasises the objects are copyable.
/// The empty base class optimization applies.
/// Any derived class of copyable should be a value type.
	class Copyable
	{
	protected:
		Copyable() = default;
		~Copyable() = default;
	};
}
