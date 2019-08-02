from pylibyaml import parse

import pytest


def test_string():
    assert parse("foo") == "foo"


def test_dict():
    assert parse("foo: bar\negg: spam") == {"foo": "bar", "egg": "spam"}


def test_none_as_empty():
    assert parse("foo:") == {"foo": ""}


def test_int_as_string():
    assert parse("foo: 1") == {"foo": "1"}


def test_list():
    assert parse("- foo\n- bar") == ["foo", "bar"]


def test_dict_in_list():
    assert parse("- foo: bar\n- egg") == [{"foo": "bar"}, "egg"]


def test_dict_in_dict():
    assert parse("foo: {egg: spam}") == {"foo": {"egg": "spam"}}


def test_list_in_dict():
    assert parse("foo:\n - bar\n - egg\nspam: stuff") == {"foo": ["bar", "egg"], "spam": "stuff"}


def test_unhashable_key():
    with pytest.raises(TypeError):
        parse("- {{}}")
